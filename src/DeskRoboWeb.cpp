#include "DeskRoboWeb.h"

#include <Arduino.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

#include "DeskRoboMVP.h"
#include "DeskRoboAudio.h"
#include "LVGL_Arduino/Display_ST77916.h"
#include "LVGL_Arduino/Gyro_QMI8658.h"
#include "LVGL_Arduino/BAT_Driver.h"
#include "LVGL_Arduino/SD_Card.h"

namespace {
WebServer server(80);

const char *kApSsid = "DeskRobo-Setup";

const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>DeskRobo</title>
<style>
body{margin:0;background:#0a0d14;color:#d8e0ff;font-family:Verdana,Segoe UI,sans-serif}
.wrap{max-width:820px;margin:auto;padding:16px}
.card{background:#131a2a;border:1px solid #2a3654;border-radius:14px;padding:14px;margin:10px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:8px}
button,select{background:#1f2a46;color:#dce7ff;border:1px solid #3d4f7d;border-radius:10px;padding:10px}
button:active{transform:scale(.98)}
input{width:100%;background:#0f1526;color:#e4ebff;border:1px solid #31416b;border-radius:10px;padding:10px}
small{opacity:.8}
</style></head><body><div class=wrap>
<h2>DeskRobo Control</h2>
<div class=card><div id=state>State: ...</div><small>AP: DeskRobo-Setup (open, no password)</small></div>
<div class=card><h3>Emotion</h3><div class=grid id=emo></div></div>
<div class=card><h3>Eye Style</h3>
<div class=grid>
<button onclick="setStyle('EVE')">EVE (WALL-E)</button>
</div>
<small>Style wirkt sofort. Optional mit Save Tune persistent speichern.</small>
</div>
<div class=card><h3>Backlight</h3>
<input id=bl type=range min=0 max=100 step=1 oninput="blv.textContent=this.value+'%'">
<div style="margin-top:8px" class=grid>
<button onclick="setBacklight()">Apply Brightness</button>
</div>
<small>Aktuell: <span id=blv>--%</span></small>
</div>
<div class=card><h3>Audio Test</h3>
<div id=audioState>audio: ...</div>
<div style="margin-top:8px" class=grid>
<button onclick="beep(880,120)">Beep Leise</button>
<button onclick="beep(660,200)">Beep Mittel</button>
</div>
<div style="margin-top:8px" class=grid>
<input id=beepLvl type=range min=200 max=4000 step=50 oninput="beepLvlTxt.textContent=this.value">
<button onclick="setBeepLevel()">Set Beep Level</button>
</div>
<small>Beep-Level: <span id=beepLvlTxt>1600</span></small>
</div>
<div class=card><h3>EVE Eyes (Left/Right)</h3>
<div class=grid><select id=leftEye></select><select id=rightEye></select></div>
<div style="margin-top:8px"><button onclick="setEyes()">Apply Eye Pair</button></div>
</div>
<div class=card><h3>Idle Tuning (No Reflash)</h3>
<div class=grid>
<input id=t_drift_amp_px type=number placeholder="drift_amp_px">
<input id=t_saccade_amp_px type=number placeholder="saccade_amp_px">
<input id=t_saccade_min_ms type=number placeholder="saccade_min_ms">
<input id=t_saccade_max_ms type=number placeholder="saccade_max_ms">
<input id=t_blink_interval_ms type=number placeholder="blink_interval_ms">
<input id=t_blink_duration_ms type=number placeholder="blink_duration_ms">
<input id=t_double_blink_chance_pct type=number placeholder="double_blink_chance_pct">
<input id=t_glow_pulse_amp type=number placeholder="glow_pulse_amp">
<input id=t_glow_pulse_period_ms type=number placeholder="glow_pulse_period_ms">
</div>
<div style="margin-top:8px" class=grid>
<button onclick="applyTune()">Apply Tune</button>
<button onclick="saveTune()">Save Tune</button>
<button onclick="loadTune()">Load Tune</button>
</div>
<div id=tuneState style="margin-top:8px;opacity:.9"></div>
</div>
<div class=card><h3>Events</h3>
<div class=grid>
<button onclick="evt('CALL')">Incoming Call</button><button onclick="evt('MAIL')">Mail</button>
<button onclick="evt('TEAMS')">Teams</button><button onclick="evt('LOUD')">Loud</button>
<button onclick="evt('VERY_LOUD')">Very Loud</button><button onclick="evt('TILT')">Tilt</button>
<button onclick="evt('SHAKE')">Shake</button><button onclick="evt('QUIET')">Quiet</button>
</div></div>
<div class=card><h3>OTA</h3><input type=file id=f><button onclick=ota()>Upload Firmware</button><div id=otaState></div></div>
</div>
<script>
const emos=['IDLE','HAPPY','SAD','ANGRY','WOW','SLEEPY','CONFUSED','EXCITED'];
const box=document.getElementById('emo');
const leftSel=document.getElementById('leftEye');
const rightSel=document.getElementById('rightEye');
emos.forEach(e=>{const b=document.createElement('button');b.textContent=e;b.onclick=()=>setEmo(e);box.appendChild(b);});
emos.forEach(e=>{const o1=document.createElement('option');o1.value=e;o1.textContent='Left: '+e;leftSel.appendChild(o1);
                 const o2=document.createElement('option');o2.value=e;o2.textContent='Right: '+e;rightSel.appendChild(o2);});
leftSel.value='IDLE';rightSel.value='IDLE';
async function setEmo(name){await fetch('/api/emotion?name='+name+'&hold=3500',{method:'POST'});refresh();}
async function setStyle(name){await fetch('/api/style?name='+name,{method:'POST'});refresh();}
async function setBacklight(){
 const v=parseInt(document.getElementById('bl').value,10);
 await fetch('/api/backlight?value='+v,{method:'POST'});
 await refreshBacklight();
}
async function setEyes(){await fetch('/api/eyes?left='+leftSel.value+'&right='+rightSel.value+'&hold=5000',{method:'POST'});refresh();}
async function evt(name){await fetch('/api/event?name='+name,{method:'POST'});refresh();}
async function refresh(){const r=await fetch('/api/status');document.getElementById('state').textContent='State: '+await r.text();}
async function refreshBacklight(){
 const r=await fetch('/api/backlight');
 const t=await r.text();
 const v=parseInt(t,10);
 if(!Number.isNaN(v)){document.getElementById('bl').value=v;document.getElementById('blv').textContent=v+'%';}
}
async function refreshAudio(){
 const r=await fetch('/api/audio');
 const a=await r.json();
 document.getElementById('audioState').textContent='mic_ok='+a.mic_ok+' spk_ok='+a.speaker_ok+' rms='+a.rms.toFixed(4)+' peak='+a.peak.toFixed(4);
}
async function beep(hz,ms){
 await fetch('/api/audio/beep?hz='+hz+'&ms='+ms,{method:'POST'});
 await refreshAudio();
}
async function setBeepLevel(){
 const v=parseInt(document.getElementById('beepLvl').value,10);
 await fetch('/api/audio/level?value='+v,{method:'POST'});
 await refreshAudio();
}
async function refreshTune(){const r=await fetch('/api/tune/get');const t=await r.json();
 for(const k of Object.keys(t)){const el=document.getElementById('t_'+k);if(el)el.value=t[k];}}
async function applyTune(){
 const keys=['drift_amp_px','saccade_amp_px','saccade_min_ms','saccade_max_ms','blink_interval_ms','blink_duration_ms','double_blink_chance_pct','glow_pulse_amp','glow_pulse_period_ms'];
 let ok=0;
 for(const k of keys){const el=document.getElementById('t_'+k);if(!el)continue;
   const v=parseInt(el.value,10);if(Number.isNaN(v))continue;
   const r=await fetch('/api/tune/set?key='+k+'&value='+v,{method:'POST'});if(r.ok)ok++;}
 document.getElementById('tuneState').textContent='Applied '+ok+' params';
 refresh();
}
async function saveTune(){const r=await fetch('/api/tune/save',{method:'POST'});document.getElementById('tuneState').textContent=await r.text();}
async function loadTune(){const r=await fetch('/api/tune/load',{method:'POST'});document.getElementById('tuneState').textContent=await r.text();await refreshTune();refresh();}
async function ota(){const fi=document.getElementById('f');if(!fi.files.length)return;
 const fd=new FormData();fd.append('firmware',fi.files[0]);document.getElementById('otaState').textContent='Uploading...';
 const r=await fetch('/api/ota',{method:'POST',body:fd});document.getElementById('otaState').textContent=await r.text();}
async function loopRefresh(){try{await refresh();}catch(e){}setTimeout(loopRefresh, 2000);}
refreshTune();refreshBacklight();refreshAudio();loopRefresh();
</script></body></html>
)HTML";

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

const char *emotionToName(DeskRoboEmotion e) {
  switch (e) {
    case DESKROBO_EMOTION_HAPPY: return "HAPPY";
    case DESKROBO_EMOTION_SAD: return "SAD";
    case DESKROBO_EMOTION_ANGRY: return "ANGRY";
    case DESKROBO_EMOTION_WOW: return "WOW";
    case DESKROBO_EMOTION_SLEEPY: return "SLEEPY";
    case DESKROBO_EMOTION_CONFUSED: return "CONFUSED";
    case DESKROBO_EMOTION_EXCITED: return "EXCITED";
    case DESKROBO_EMOTION_IDLE:
    default: return "IDLE";
  }
}

void handleRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", kIndexHtml);
  });

  server.on("/api/status", HTTP_GET, []() {
    const float bat_v = BAT_Get_Volts();
    char buf[256];
    snprintf(buf, sizeof(buf), "emotion=%s style=%s bat=%.2fV sd=%uMB flash=%uMB gyro=%s ax=%.2f ay=%.2f az=%.2f",
             emotionToName(DeskRobo_GetEmotion()), DeskRobo_GetStyleName(),
             bat_v, SDCard_Size, Flash_Size, QMI8658_IsReady() ? "on" : "off", Accel.x, Accel.y, Accel.z);
    server.send(200, "text/plain", buf);
  });

  server.on("/api/hw", HTTP_GET, []() {
    const float bat_v = BAT_Get_Volts();
    char buf[220];
    snprintf(buf, sizeof(buf),
             "{\"bat_v\":%.3f,\"sd_mb\":%u,\"flash_mb\":%u,\"sd_present\":%s,\"gyro_ready\":%s}",
             bat_v, SDCard_Size, Flash_Size, (SDCard_Size > 0) ? "true" : "false",
             QMI8658_IsReady() ? "true" : "false");
    server.send(200, "application/json", buf);
  });

  server.on("/api/emotion", HTTP_POST, []() {
    DeskRoboEmotion emotion;
    const String name = server.arg("name");
    const uint32_t hold = server.hasArg("hold") ? server.arg("hold").toInt() : 3000;
    if (!parseEmotion(name, emotion)) {
      server.send(400, "text/plain", "invalid emotion");
      return;
    }
    DeskRobo_SetEmotion(emotion, hold);
    server.send(200, "text/plain", "ok");
  });

  server.on("/api/eyes", HTTP_POST, []() {
    DeskRoboEmotion left;
    DeskRoboEmotion right;
    const String left_name = server.arg("left");
    const String right_name = server.arg("right");
    const uint32_t hold = server.hasArg("hold") ? server.arg("hold").toInt() : 5000;
    if (!parseEmotion(left_name, left) || !parseEmotion(right_name, right)) {
      server.send(400, "text/plain", "invalid eyes");
      return;
    }
    DeskRobo_SetEyePair(left, right, hold);
    server.send(200, "text/plain", "ok");
  });

  server.on("/api/style", HTTP_POST, []() {
    const String name = server.arg("name");
    if (!DeskRobo_SetStyleByName(name.c_str())) {
      server.send(400, "text/plain", "invalid style");
      return;
    }
    server.send(200, "text/plain", "ok");
  });

  server.on("/api/backlight", HTTP_GET, []() {
    server.send(200, "text/plain", String((int) LCD_Backlight));
  });

  server.on("/api/backlight", HTTP_POST, []() {
    const int v = constrain(server.arg("value").toInt(), 0, 100);
    Set_Backlight((uint8_t) v);
    server.send(200, "text/plain", String(v));
  });

  server.on("/api/audio", HTTP_GET, []() {
    char buf[180];
    snprintf(buf, sizeof(buf),
             "{\"mic_ok\":%s,\"speaker_ok\":%s,\"rms\":%.6f,\"peak\":%.6f}",
             DeskRoboAudio_MicReady() ? "true" : "false",
             DeskRoboAudio_SpeakerReady() ? "true" : "false",
             DeskRoboAudio_MicRms(), DeskRoboAudio_MicPeak());
    server.send(200, "application/json", buf);
  });

  server.on("/api/audio/beep", HTTP_POST, []() {
    const uint16_t hz = (uint16_t) constrain(server.arg("hz").toInt(), 80, 4000);
    const uint16_t ms = (uint16_t) constrain(server.arg("ms").toInt(), 20, 1200);
    const bool ok = DeskRoboAudio_Beep(hz, ms);
    server.send(ok ? 200 : 500, "text/plain", ok ? "ok" : "speaker not ready");
  });

  server.on("/api/audio/level", HTTP_POST, []() {
    const uint16_t v = (uint16_t) constrain(server.arg("value").toInt(), 200, 8000);
    DeskRoboAudio_SetBeepLevel(v);
    server.send(200, "text/plain", String(v));
  });

  server.on("/api/tune/get", HTTP_GET, []() {
    server.send(200, "application/json", DeskRobo_GetTuningJson());
  });

  server.on("/api/tune/set", HTTP_POST, []() {
    const String key = server.arg("key");
    const int value = server.arg("value").toInt();
    if (!DeskRobo_SetTuning(key.c_str(), value)) {
      server.send(400, "text/plain", "invalid tuning key");
      return;
    }
    server.send(200, "text/plain", "ok");
  });

  server.on("/api/tune/save", HTTP_POST, []() {
    const bool ok = DeskRobo_SaveTuning();
    server.send(ok ? 200 : 500, "text/plain", ok ? "saved" : "save failed");
  });

  server.on("/api/tune/load", HTTP_POST, []() {
    const bool ok = DeskRobo_LoadTuning();
    server.send(ok ? 200 : 500, "text/plain", ok ? "loaded" : "load failed");
  });

  server.on("/api/event", HTTP_POST, []() {
    DeskRoboEventType ev;
    const String name = server.arg("name");
    if (!parseEvent(name, ev)) {
      server.send(400, "text/plain", "invalid event");
      return;
    }
    DeskRobo_PushEvent(ev);
    server.send(200, "text/plain", "ok");
  });

  server.on(
      "/api/ota", HTTP_POST,
      []() {
        const bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain", ok ? "OTA done. Rebooting..." : "OTA failed");
        if (ok) {
          delay(200);
          ESP.restart();
        }
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Update.begin(UPDATE_SIZE_UNKNOWN);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          Update.write(upload.buf, upload.currentSize);
        } else if (upload.status == UPLOAD_FILE_END) {
          Update.end(true);
        }
      });

  server.onNotFound([]() {
    server.send(404, "text/plain", "not found");
  });
}
}  // namespace

void DeskRoboWeb_Init() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.softAP(kApSsid);
  Serial.printf("[WEB] AP started: %s IP=%s\n", kApSsid, WiFi.softAPIP().toString().c_str());

  WiFi.begin("home", "mon015146659541");
  Serial.printf("[WEB] Connecting to home wlan for time sync...\n");

  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");

  handleRoutes();
  server.begin();
  Serial.println("[WEB] HTTP server started");
}

void DeskRoboWeb_Loop() {
  server.handleClient();
}
