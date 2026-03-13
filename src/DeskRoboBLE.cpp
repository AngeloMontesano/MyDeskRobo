#include "DeskRoboBLE.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#include "DeskRoboMVP.h"
#include "LVGL_Arduino/Display_ST77916.h"

namespace {
constexpr const char *kDeviceName = "MyRoboDesk";
constexpr const char *kServiceUuid = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
constexpr const char *kIoCharUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

bool g_device_connected = false;
QueueHandle_t g_cmd_queue = nullptr;
BLECharacteristic *g_io_char = nullptr;
bool g_conn_state_initialized = false;
bool g_last_conn_state = false;

enum CmdType : uint8_t {
  CMD_NONE = 0,
  CMD_SET_EMOTION = 1,
  CMD_PUSH_EVENT = 2,
  CMD_SET_EYES = 3,
  CMD_SET_TIME = 4,
  CMD_SET_STYLE = 5,
  CMD_SET_STATUS_LABEL = 6,
  CMD_SET_BACKLIGHT = 7,
  CMD_SET_TUNING = 8,
  CMD_SAVE_TUNING = 9,
  CMD_LOAD_TUNING = 10,
};

struct BleCmd {
  uint8_t type;
  uint8_t a;
  uint8_t b;
  uint32_t hold_ms;
};

bool enqueueCmd(const BleCmd &cmd) {
  if (!g_cmd_queue) return false;
  return xQueueSend(g_cmd_queue, &cmd, 0) == pdTRUE;
}

void notifyText(const char *text) {
  if (!g_device_connected || !g_io_char || !text) return;
  g_io_char->setValue(text);
  g_io_char->notify();
}

void notifyAck(uint32_t seq, bool ok) {
  char buf[32];
  snprintf(buf, sizeof(buf), "ACK:%lu:%u", (unsigned long)seq, ok ? 1u : 0u);
  notifyText(buf);
}

void notifyPong(uint32_t seq) {
  char buf[24];
  snprintf(buf, sizeof(buf), "PONG:%lu", (unsigned long)seq);
  notifyText(buf);
}

bool parseUint32Strict(const String &in, uint32_t &out) {
  if (in.length() == 0) return false;
  uint32_t v = 0;
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in[i];
    if (c < '0' || c > '9') return false;
    const uint32_t d = (uint32_t)(c - '0');
    if (v > (0xFFFFFFFFu - d) / 10u) return false;
    v = (v * 10u) + d;
  }
  out = v;
  return true;
}

bool parseEmotion(const String &name, DeskRoboEmotion &out) {
  if (name == "IDLE") out = DESKROBO_EMOTION_IDLE;
  else if (name == "HAPPY") out = DESKROBO_EMOTION_HAPPY;
  else if (name == "SAD") out = DESKROBO_EMOTION_SAD;
  else if (name == "ANGRY") out = DESKROBO_EMOTION_ANGRY;
  else if (name == "WOW") out = DESKROBO_EMOTION_WOW;
  else if (name == "SLEEPY") out = DESKROBO_EMOTION_SLEEPY;
  else if (name == "CONFUSED") out = DESKROBO_EMOTION_CONFUSED;
  else if (name == "EXCITED") out = DESKROBO_EMOTION_EXCITED;
  else if (name == "DIZZY") out = DESKROBO_EMOTION_DIZZY;
  else if (name == "MAIL") out = DESKROBO_EMOTION_MAIL;
  else if (name == "CALL") out = DESKROBO_EMOTION_CALL;
  else if (name == "SHAKE") out = DESKROBO_EMOTION_SHAKE;
  else return false;
  return true;
}

bool parseEvent(const String &name, DeskRoboEventType &out) {
  if (name == "QUIET") out = DESKROBO_EVENT_AUDIO_QUIET;
  else if (name == "LOUD") out = DESKROBO_EVENT_AUDIO_LOUD;
  else if (name == "VERY_LOUD") out = DESKROBO_EVENT_AUDIO_VERY_LOUD;
  else if (name == "TILT") out = DESKROBO_EVENT_MOTION_TILT;
  else if (name == "SHAKE") out = DESKROBO_EVENT_MOTION_SHAKE;
  else if (name == "CALL") out = DESKROBO_EVENT_PC_CALL;
  else if (name == "MAIL") out = DESKROBO_EVENT_PC_MAIL;
  else if (name == "TEAMS") out = DESKROBO_EVENT_PC_TEAMS;
  else return false;
  return true;
}

enum TuneKeyId : uint8_t {
  TUNE_KEY_NONE = 0,
  TUNE_KEY_DRIFT_AMP = 1,
  TUNE_KEY_SACCADE_AMP = 2,
  TUNE_KEY_SACCADE_MIN = 3,
  TUNE_KEY_SACCADE_MAX = 4,
  TUNE_KEY_BLINK_INTERVAL = 5,
  TUNE_KEY_BLINK_DURATION = 6,
  TUNE_KEY_DBL_BLINK_PCT = 7,
  TUNE_KEY_GLOW_AMP = 8,
  TUNE_KEY_GLOW_PERIOD = 9,
  TUNE_KEY_GYRO_TILT_XY = 10,
  TUNE_KEY_GYRO_TILT_Z = 11,
  TUNE_KEY_GYRO_TILT_COOLDOWN = 12,
};

bool parseBoolToken(const String &in, bool &out) {
  if ((in == "1") || (in == "TRUE") || (in == "ON")) {
    out = true;
    return true;
  }
  if ((in == "0") || (in == "FALSE") || (in == "OFF")) {
    out = false;
    return true;
  }
  return false;
}

bool parseStyleCode(const String &name, uint8_t &out) {
  if ((name == "EVE") || (name == "EVE_CINEMATIC")) {
    out = (uint8_t)DESKROBO_STYLE_EVE;
    return true;
  }
  if (name == "FLUX") {
    out = (uint8_t)DESKROBO_STYLE_FLUX;
    return true;
  }
  return false;
}

bool parseTuneKeyId(const String &name, uint8_t &out) {
  if (name == "DRIFT_AMP_PX") out = TUNE_KEY_DRIFT_AMP;
  else if (name == "SACCADE_AMP_PX") out = TUNE_KEY_SACCADE_AMP;
  else if (name == "SACCADE_MIN_MS") out = TUNE_KEY_SACCADE_MIN;
  else if (name == "SACCADE_MAX_MS") out = TUNE_KEY_SACCADE_MAX;
  else if (name == "BLINK_INTERVAL_MS") out = TUNE_KEY_BLINK_INTERVAL;
  else if (name == "BLINK_DURATION_MS") out = TUNE_KEY_BLINK_DURATION;
  else if (name == "DOUBLE_BLINK_CHANCE_PCT") out = TUNE_KEY_DBL_BLINK_PCT;
  else if (name == "GLOW_PULSE_AMP") out = TUNE_KEY_GLOW_AMP;
  else if (name == "GLOW_PULSE_PERIOD_MS") out = TUNE_KEY_GLOW_PERIOD;
  else if (name == "GYRO_TILT_XY_PCT") out = TUNE_KEY_GYRO_TILT_XY;
  else if (name == "GYRO_TILT_Z_PCT") out = TUNE_KEY_GYRO_TILT_Z;
  else if (name == "GYRO_TILT_COOLDOWN_MS") out = TUNE_KEY_GYRO_TILT_COOLDOWN;
  else return false;
  return true;
}

const char *tuneKeyName(uint8_t key_id) {
  switch (key_id) {
    case TUNE_KEY_DRIFT_AMP:
      return "drift_amp_px";
    case TUNE_KEY_SACCADE_AMP:
      return "saccade_amp_px";
    case TUNE_KEY_SACCADE_MIN:
      return "saccade_min_ms";
    case TUNE_KEY_SACCADE_MAX:
      return "saccade_max_ms";
    case TUNE_KEY_BLINK_INTERVAL:
      return "blink_interval_ms";
    case TUNE_KEY_BLINK_DURATION:
      return "blink_duration_ms";
    case TUNE_KEY_DBL_BLINK_PCT:
      return "double_blink_chance_pct";
    case TUNE_KEY_GLOW_AMP:
      return "glow_pulse_amp";
    case TUNE_KEY_GLOW_PERIOD:
      return "glow_pulse_period_ms";
    case TUNE_KEY_GYRO_TILT_XY:
      return "gyro_tilt_xy_pct";
    case TUNE_KEY_GYRO_TILT_Z:
      return "gyro_tilt_z_pct";
    case TUNE_KEY_GYRO_TILT_COOLDOWN:
      return "gyro_tilt_cooldown_ms";
    default:
      return nullptr;
  }
}

bool enqueueEmotionCode(uint8_t code, uint32_t hold_override_ms = 0) {
  switch (code) {
    case 0x01:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_HAPPY, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x02:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_SAD, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x03:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_ANGRY, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x04:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_WOW, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x05:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_SLEEPY, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x06:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_EXCITED, 0,
                         hold_override_ms > 0 ? hold_override_ms : 2500});
    case 0x07:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_HAPPY, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x0A:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_EXCITED, 0,
                         hold_override_ms > 0 ? hold_override_ms : 5000});
    case 0x0B:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_HAPPY, 0,
                         hold_override_ms > 0 ? hold_override_ms : 5000});
    case 0x0C:
    case 0x0D:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_CONFUSED, 0,
                         hold_override_ms > 0 ? hold_override_ms : 5000});
    case 0x0E:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_EXCITED, 0,
                         hold_override_ms > 0 ? hold_override_ms : 2500});
    case 0x0F:
    case 0x10:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_CONFUSED, 0,
                         hold_override_ms > 0 ? hold_override_ms : 3000});
    case 0x00:
    case 0xFF:
    default:
      return enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_IDLE, 0,
                         hold_override_ms > 0 ? hold_override_ms : 1500});
  }
}

void handleBinaryCode(uint8_t code) {
  Serial.printf("[BLE] rx code=0x%02X (%u)\n", code, code);
  (void)enqueueEmotionCode(code);
}

void handleTextCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  String clean;
  clean.reserve(cmd.length());
  for (char c : cmd) {
    if (c != '\r' && c != '\n' && c != '\0') clean += c;
  }

  String u = clean;
  u.toUpperCase();

  if (u.startsWith("PING:")) {
    uint32_t seq = 0;
    if (parseUint32Strict(u.substring(5), seq)) {
      notifyPong(seq);
    }
    return;
  }

  if (u.startsWith("EMO:")) {
    const int p1 = u.indexOf(':', 4);
    const int p2 = (p1 >= 0) ? u.indexOf(':', p1 + 1) : -1;

    if (p1 < 0) return;

    uint32_t seq = 0;
    uint32_t code_u32 = 0;
    uint32_t hold_ms = 0;
    const bool seq_ok = parseUint32Strict(u.substring(4, p1), seq);

    bool code_ok = false;
    if (p2 < 0) {
      code_ok = parseUint32Strict(u.substring(p1 + 1), code_u32);
    } else {
      code_ok = parseUint32Strict(u.substring(p1 + 1, p2), code_u32);
      uint32_t hold_tmp = 0;
      if (parseUint32Strict(u.substring(p2 + 1), hold_tmp)) {
        hold_ms = hold_tmp;
      }
    }

    bool ok = false;
    if (seq_ok && code_ok && code_u32 <= 255u) {
      ok = enqueueEmotionCode((uint8_t)code_u32, hold_ms);
      Serial.printf("[BLE] rx emo seq=%lu code=%lu hold=%lu ok=%u\n",
                    (unsigned long)seq, (unsigned long)code_u32,
                    (unsigned long)hold_ms, ok ? 1u : 0u);
      notifyAck(seq, ok);
    }
    return;
  }

  if (u.startsWith("TIME:")) {
    const int p1 = u.indexOf(':', 5);
    if (p1 < 0) return;

    uint32_t seq = 0;
    uint32_t epoch_s = 0;
    const bool seq_ok = parseUint32Strict(u.substring(5, p1), seq);
    const bool epoch_ok = parseUint32Strict(u.substring(p1 + 1), epoch_s);

    bool ok = false;
    if (seq_ok && epoch_ok) {
      ok = enqueueCmd({CMD_SET_TIME, 0, 0, epoch_s});
      Serial.printf("[BLE] rx time seq=%lu unix=%lu ok=%u\n",
                    (unsigned long)seq, (unsigned long)epoch_s, ok ? 1u : 0u);
      notifyAck(seq, ok);
    }
    return;
  }

  if (u.startsWith("CMD:")) {
    const int p1 = u.indexOf(':', 4);
    const int p2 = (p1 >= 0) ? u.indexOf(':', p1 + 1) : -1;
    if (p1 < 0 || p2 < 0) return;

    uint32_t seq = 0;
    if (!parseUint32Strict(u.substring(4, p1), seq)) return;

    const String action = u.substring(p1 + 1, p2);
    const String payload = u.substring(p2 + 1);
    bool ok = false;

    if (action == "STYLE") {
      uint8_t style_code = 0;
      if (parseStyleCode(payload, style_code)) {
        ok = enqueueCmd({CMD_SET_STYLE, style_code, 0, 0});
      }
    } else if (action == "STATUS_LABEL") {
      bool visible = false;
      if (parseBoolToken(payload, visible)) {
        ok = enqueueCmd({CMD_SET_STATUS_LABEL, (uint8_t)(visible ? 1 : 0), 0, 0});
      }
    } else if (action == "BACKLIGHT") {
      uint32_t value = 0;
      if (parseUint32Strict(payload, value)) {
        ok = enqueueCmd({CMD_SET_BACKLIGHT, (uint8_t)constrain((int)value, 0, 100), 0, 0});
      }
    } else if (action == "TUNE") {
      const int p3 = u.indexOf(':', p2 + 1);
      if (p3 > 0) {
        const String key_name = u.substring(p2 + 1, p3);
        uint32_t value = 0;
        uint8_t key_id = TUNE_KEY_NONE;
        if (parseTuneKeyId(key_name, key_id) && parseUint32Strict(u.substring(p3 + 1), value)) {
          ok = enqueueCmd({CMD_SET_TUNING, key_id, 0, value});
        }
      }
    } else if (action == "TUNE_SAVE") {
      ok = enqueueCmd({CMD_SAVE_TUNING, 0, 0, 0});
    } else if (action == "TUNE_LOAD") {
      ok = enqueueCmd({CMD_LOAD_TUNING, 0, 0, 0});
    } else if (action == "EVENT") {
      DeskRoboEventType ev;
      if (parseEvent(payload, ev)) {
        ok = enqueueCmd({CMD_PUSH_EVENT, (uint8_t)ev, 0, 0});
      }
    } else if (action == "EMOTION") {
      const int p3 = u.indexOf(':', p2 + 1);
      String emo_name = payload;
      uint32_t hold_ms = 3500;
      if (p3 > 0) {
        emo_name = u.substring(p2 + 1, p3);
        uint32_t hold_tmp = 0;
        if (parseUint32Strict(u.substring(p3 + 1), hold_tmp)) {
          hold_ms = hold_tmp;
        }
      }
      DeskRoboEmotion emo;
      if (parseEmotion(emo_name, emo)) {
        ok = enqueueCmd({CMD_SET_EMOTION, (uint8_t)emo, 0, hold_ms});
      }
    } else if (action == "EYES") {
      const int p3 = u.indexOf(':', p2 + 1);
      const int p4 = (p3 >= 0) ? u.indexOf(':', p3 + 1) : -1;
      if (p3 > 0 && p4 > 0) {
        const String left_name = u.substring(p2 + 1, p3);
        const String right_name = u.substring(p3 + 1, p4);
        uint32_t hold_ms = 0;
        DeskRoboEmotion left;
        DeskRoboEmotion right;
        if (parseEmotion(left_name, left) && parseEmotion(right_name, right) &&
            parseUint32Strict(u.substring(p4 + 1), hold_ms)) {
          ok = enqueueCmd({CMD_SET_EYES, (uint8_t)left, (uint8_t)right, hold_ms > 0 ? hold_ms : 5000u});
        }
      }
    }

    notifyAck(seq, ok);
    return;
  }
  if (u.startsWith("EVENT:")) {
    const String name = u.substring(6);
    DeskRoboEventType ev;
    if (parseEvent(name, ev)) {
      (void)enqueueCmd({CMD_PUSH_EVENT, (uint8_t)ev, 0, 0});
    }
    return;
  }

  if (u.startsWith("EMOTION:")) {
    const int p = u.indexOf(':', 8);
    const String name = (p < 0) ? u.substring(8) : u.substring(8, p);
    const uint32_t hold_ms =
        (p < 0) ? 3500u : (uint32_t)u.substring(p + 1).toInt();
    DeskRoboEmotion emo;
    if (parseEmotion(name, emo)) {
      (void)enqueueCmd(
          {CMD_SET_EMOTION, (uint8_t)emo, 0, hold_ms > 0 ? hold_ms : 3500u});
    }
    return;
  }

  if (u.startsWith("EYES:")) {
    const int p1 = u.indexOf(':', 5);
    const int p2 = (p1 >= 0) ? u.indexOf(':', p1 + 1) : -1;
    if (p1 < 0 || p2 < 0) return;

    const String left_name = u.substring(5, p1);
    const String right_name = u.substring(p1 + 1, p2);
    const uint32_t hold_ms = (uint32_t)u.substring(p2 + 1).toInt();
    DeskRoboEmotion left;
    DeskRoboEmotion right;
    if (parseEmotion(left_name, left) && parseEmotion(right_name, right)) {
      (void)enqueueCmd({CMD_SET_EYES, (uint8_t)left, (uint8_t)right,
                        hold_ms > 0 ? hold_ms : 5000u});
    }
    return;
  }
}

class DeskRoboBleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    g_device_connected = true;
    (void)server;
    Serial.println("[BLE] client connected");
  }

  void onDisconnect(BLEServer *server) override {
    g_device_connected = false;
    Serial.println("[BLE] client disconnected");
    if (server) server->startAdvertising();
    Serial.println("[BLE] advertising restarted");
  }
};

class DeskRoboBleCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue();
    if (value.length() == 0) return;

    if (value.length() == 1) {
      handleBinaryCode((uint8_t)value[0]);
      return;
    }

    handleTextCommand(value);
  }
};
}  // namespace

void DeskRoboBLE_Init() {
  g_cmd_queue = xQueueCreate(24, sizeof(BleCmd));

  BLEDevice::init(kDeviceName);
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new DeskRoboBleServerCallbacks());

  BLEService *service = server->createService(kServiceUuid);
  BLECharacteristic *characteristic = service->createCharacteristic(
      kIoCharUuid,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR |
          BLECharacteristic::PROPERTY_NOTIFY);
  characteristic->setCallbacks(new DeskRoboBleCharCallbacks());
  g_io_char = characteristic;

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("[BLE] advertising started");
}

void DeskRoboBLE_Loop() {
  BleCmd cmd;
  while (g_cmd_queue && xQueueReceive(g_cmd_queue, &cmd, 0) == pdTRUE) {
    switch (cmd.type) {
      case CMD_SET_EMOTION:
        Serial.printf("[BLE] apply emotion=%u hold=%lu\n", (unsigned)cmd.a,
                      (unsigned long)cmd.hold_ms);
        DeskRobo_SetEmotion((DeskRoboEmotion)cmd.a, cmd.hold_ms);
        break;
      case CMD_PUSH_EVENT:
        DeskRobo_PushEvent((DeskRoboEventType)cmd.a);
        break;
      case CMD_SET_EYES:
        DeskRobo_SetEyePair((DeskRoboEmotion)cmd.a, (DeskRoboEmotion)cmd.b,
                            cmd.hold_ms);
        break;
      case CMD_SET_TIME: {
        // Keep the same TZ behavior as NTP setup so local time display matches.
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset();

        const time_t epoch = (time_t)cmd.hold_ms;
        struct timeval tv = {.tv_sec = epoch, .tv_usec = 0};
        const bool ok = (settimeofday(&tv, nullptr) == 0);
        Serial.printf("[BLE] apply time unix=%lu ok=%u\n",
                      (unsigned long)cmd.hold_ms, ok ? 1u : 0u);
        break;
      }
      case CMD_SET_STYLE: {
        if (cmd.a == (uint8_t)DESKROBO_STYLE_FLUX) {
          (void)DeskRobo_SetStyleByName("FLUX");
        } else {
          (void)DeskRobo_SetStyleByName("EVE_CINEMATIC");
        }
        break;
      }
      case CMD_SET_STATUS_LABEL:
        DeskRobo_SetStatusLabelVisible(cmd.a != 0);
        break;
      case CMD_SET_BACKLIGHT:
        Set_Backlight((uint8_t)constrain((int)cmd.a, 0, 100));
        break;
      case CMD_SET_TUNING: {
        const char *key = tuneKeyName(cmd.a);
        if (key) {
          (void)DeskRobo_SetTuning(key, (int)cmd.hold_ms);
        }
        break;
      }
      case CMD_SAVE_TUNING:
        (void)DeskRobo_SaveTuning();
        break;
      case CMD_LOAD_TUNING:
        (void)DeskRobo_LoadTuning();
        break;
      default:
        break;
    }
  }

  if (!g_conn_state_initialized || (g_last_conn_state != g_device_connected)) {
    g_conn_state_initialized = true;
    g_last_conn_state = g_device_connected;
    DeskRobo_SetBleConnected(g_device_connected);
  }
}
