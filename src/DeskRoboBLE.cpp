#include "DeskRoboBLE.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "DeskRoboMVP.h"

namespace {
constexpr const char *kDeviceName = "DeskRobot";
constexpr const char *kServiceUuid = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
constexpr const char *kNotifyCharUuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

bool g_device_connected = false;
bool g_last_connected = false;
QueueHandle_t g_cmd_queue = nullptr;

enum CmdType : uint8_t {
  CMD_NONE = 0,
  CMD_SET_EMOTION = 1,
  CMD_PUSH_EVENT = 2,
  CMD_SET_EYES = 3,
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

bool parseEmotion(const String &name, DeskRoboEmotion &out) {
  if (name == "IDLE") out = DESKROBO_EMOTION_IDLE;
  else if (name == "HAPPY") out = DESKROBO_EMOTION_HAPPY;
  else if (name == "SAD") out = DESKROBO_EMOTION_SAD;
  else if (name == "ANGRY") out = DESKROBO_EMOTION_ANGRY;
  else if (name == "WOW") out = DESKROBO_EMOTION_WOW;
  else if (name == "SLEEPY") out = DESKROBO_EMOTION_SLEEPY;
  else if (name == "CONFUSED") out = DESKROBO_EMOTION_CONFUSED;
  else if (name == "EXCITED") out = DESKROBO_EMOTION_EXCITED;
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

void handleBinaryCode(uint8_t code) {
  Serial.printf("[BLE] rx code=0x%02X (%u)\n", code, code);
  // 1-byte protocol aligned with pc_agent Emotion table:
  // 0..7 face states, 10..16 notification states
  switch (code) {
    case 0x01:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_HAPPY, 0, 3000});
      break;
    case 0x02:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_SAD, 0, 3000});
      break;
    case 0x03:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_ANGRY, 0, 3000});
      break;
    case 0x04:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_WOW, 0, 3000});
      break;
    case 0x05:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_SLEEPY, 0, 3000});
      break;
    case 0x06:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_EXCITED, 0, 2500});
      break;
    case 0x07:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_HAPPY, 0, 3000});
      break;
    case 0x0A:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_EXCITED, 0, 5000});
      break;
    case 0x0B:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_HAPPY, 0, 5000});
      break;
    case 0x0C:
    case 0x0D:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_CONFUSED, 0, 5000});
      break;
    case 0x0E:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_EXCITED, 0, 2500});
      break;
    case 0x0F:
    case 0x10:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_CONFUSED, 0, 3000});
      break;
    case 0x00:
    case 0xFF:
    default:
      enqueueCmd({CMD_SET_EMOTION, DESKROBO_EMOTION_IDLE, 0, 1500});
      break;
  }
}

void handleTextCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.startsWith("EVENT:")) {
    const String name = cmd.substring(6);
    DeskRoboEventType ev;
    if (parseEvent(name, ev)) {
      enqueueCmd({CMD_PUSH_EVENT, (uint8_t) ev, 0, 0});
    }
    return;
  }

  if (cmd.startsWith("EMOTION:")) {
    int p = cmd.indexOf(':', 8);
    const String name = (p < 0) ? cmd.substring(8) : cmd.substring(8, p);
    const uint32_t hold_ms = (p < 0) ? 3500 : (uint32_t) cmd.substring(p + 1).toInt();
    DeskRoboEmotion emo;
    if (parseEmotion(name, emo)) {
      enqueueCmd({CMD_SET_EMOTION, (uint8_t) emo, 0, hold_ms > 0 ? hold_ms : 3500});
    }
    return;
  }

  if (cmd.startsWith("EYES:")) {
    const int p1 = cmd.indexOf(':', 5);
    const int p2 = (p1 >= 0) ? cmd.indexOf(':', p1 + 1) : -1;
    const int p3 = (p2 >= 0) ? cmd.indexOf(':', p2 + 1) : -1;
    if (p1 < 0 || p2 < 0) return;

    const String left_name = cmd.substring(5, p1);
    const String right_name = cmd.substring(p1 + 1, p2);
    const uint32_t hold_ms = (p3 < 0) ? 5000 : (uint32_t) cmd.substring(p2 + 1).toInt();
    DeskRoboEmotion left;
    DeskRoboEmotion right;
    if (parseEmotion(left_name, left) && parseEmotion(right_name, right)) {
      enqueueCmd({CMD_SET_EYES, (uint8_t) left, (uint8_t) right, hold_ms > 0 ? hold_ms : 5000});
    }
    return;
  }
}

class DeskRoboBleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    g_device_connected = true;
    (void) server;
    Serial.println("[BLE] client connected");
  }

  void onDisconnect(BLEServer *server) override {
    g_device_connected = false;
    (void) server;
    Serial.println("[BLE] client disconnected");
  }
};

class DeskRoboBleCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue();
    if (value.length() == 0) return;

    if (value.length() == 1) {
      handleBinaryCode((uint8_t) value[0]);
      return;
    }

    String cmd;
    cmd.reserve(value.length());
    for (char c : value) {
      if (c != '\r' && c != '\n' && c != '\0') cmd += c;
    }
    handleTextCommand(cmd);
  }
};
}  // namespace

void DeskRoboBLE_Init() {
  g_cmd_queue = xQueueCreate(16, sizeof(BleCmd));
  BLEDevice::init(kDeviceName);
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new DeskRoboBleServerCallbacks());

  BLEService *service = server->createService(kServiceUuid);
  BLECharacteristic *characteristic = service->createCharacteristic(
      kNotifyCharUuid,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  characteristic->setCallbacks(new DeskRoboBleCharCallbacks());
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
        Serial.printf("[BLE] apply emotion=%u hold=%lu\n", (unsigned) cmd.a, (unsigned long) cmd.hold_ms);
        DeskRobo_SetEmotion((DeskRoboEmotion) cmd.a, cmd.hold_ms);
        break;
      case CMD_PUSH_EVENT:
        DeskRobo_PushEvent((DeskRoboEventType) cmd.a);
        break;
      case CMD_SET_EYES:
        DeskRobo_SetEyePair((DeskRoboEmotion) cmd.a, (DeskRoboEmotion) cmd.b, cmd.hold_ms);
        break;
      default:
        break;
    }
  }

  if (!g_device_connected && g_last_connected) {
    delay(100);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] advertising restarted");
  }
  g_last_connected = g_device_connected;
}
