#include "DeskRoboWeb.h"

#include <Arduino.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

#include "DeskRoboMVP.h"
#include "LVGL_Arduino/Gyro_QMI8658.h"

namespace {
WebServer server(80);

const char *kApSsid = "DeskRobo-Setup";
const char *kApPass = "deskrobo123";

const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>DeskRobo</title>
<style>
body{margin:0;background:#0a0d14;color:#d8e0ff;font-family:Verdana,Segoe UI,sans-serif}
.wrap{max-width:820px;margin:auto;padding:16px}
.card{background:#131a2a;border:1px solid #2a3654;border-radius:14px;padding:14px;margin:10px 0}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:8px}
button{background:#1f2a46;color:#dce7ff;border:1px solid #3d4f7d;border-radius:10px;padding:10px}
button:active{transform:scale(.98)}
input{width:100%;background:#0f1526;color:#e4ebff;border:1px solid #31416b;border-radius:10px;padding:10px}
small{opacity:.8}
</style></head><body><div class=wrap>
<h2>DeskRobo Control</h2>
<div class=card><div id=state>State: ...</div><small>AP: DeskRobo-Setup / deskrobo123</small></div>
<div class=card><h3>Emotion</h3><div class=grid id=emo></div></div>
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
const emos=['IDLE','HAPPY','SAD','ANGRY','ANGST','WOW','SLEEPY','LOVE','CONFUSED','EXCITED','ANRUF','LAUT','MAIL','DENKEN','WINK','GLITCH'];
const box=document.getElementById('emo');
emos.forEach(e=>{const b=document.createElement('button');b.textContent=e;b.onclick=()=>setEmo(e);box.appendChild(b);});
async function setEmo(name){await fetch('/api/emotion?name='+name+'&hold=3500',{method:'POST'});refresh();}
async function evt(name){await fetch('/api/event?name='+name,{method:'POST'});refresh();}
async function refresh(){const r=await fetch('/api/status');document.getElementById('state').textContent='State: '+await r.text();}
async function ota(){const fi=document.getElementById('f');if(!fi.files.length)return;
 const fd=new FormData();fd.append('firmware',fi.files[0]);document.getElementById('otaState').textContent='Uploading...';
 const r=await fetch('/api/ota',{method:'POST',body:fd});document.getElementById('otaState').textContent=await r.text();}
setInterval(refresh,1200);refresh();
</script></body></html>
)HTML";

bool parseEmotion(const String &name, DeskRoboEmotion &out) {
  if (name == "IDLE") out = DESKROBO_EMOTION_IDLE;
  else if (name == "HAPPY") out = DESKROBO_EMOTION_HAPPY;
  else if (name == "SAD") out = DESKROBO_EMOTION_SAD;
  else if (name == "ANGRY") out = DESKROBO_EMOTION_ANGRY;
  else if (name == "ANGST") out = DESKROBO_EMOTION_ANGST;
  else if (name == "WOW") out = DESKROBO_EMOTION_WOW;
  else if (name == "SLEEPY") out = DESKROBO_EMOTION_SLEEPY;
  else if (name == "LOVE") out = DESKROBO_EMOTION_LOVE;
  else if (name == "CONFUSED") out = DESKROBO_EMOTION_CONFUSED;
  else if (name == "EXCITED") out = DESKROBO_EMOTION_EXCITED;
  else if (name == "ANRUF") out = DESKROBO_EMOTION_ANRUF;
  else if (name == "LAUT") out = DESKROBO_EMOTION_LAUT;
  else if (name == "MAIL") out = DESKROBO_EMOTION_MAIL;
  else if (name == "DENKEN") out = DESKROBO_EMOTION_DENKEN;
  else if (name == "WINK") out = DESKROBO_EMOTION_WINK;
  else if (name == "GLITCH") out = DESKROBO_EMOTION_GLITCH;
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
    case DESKROBO_EMOTION_ANGST: return "ANGST";
    case DESKROBO_EMOTION_WOW: return "WOW";
    case DESKROBO_EMOTION_SLEEPY: return "SLEEPY";
    case DESKROBO_EMOTION_LOVE: return "LOVE";
    case DESKROBO_EMOTION_CONFUSED: return "CONFUSED";
    case DESKROBO_EMOTION_EXCITED: return "EXCITED";
    case DESKROBO_EMOTION_ANRUF: return "ANRUF";
    case DESKROBO_EMOTION_LAUT: return "LAUT";
    case DESKROBO_EMOTION_MAIL: return "MAIL";
    case DESKROBO_EMOTION_DENKEN: return "DENKEN";
    case DESKROBO_EMOTION_WINK: return "WINK";
    case DESKROBO_EMOTION_GLITCH: return "GLITCH";
    case DESKROBO_EMOTION_IDLE:
    default: return "IDLE";
  }
}

void handleRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", kIndexHtml);
  });

  server.on("/api/status", HTTP_GET, []() {
    char buf[180];
    snprintf(buf, sizeof(buf), "emotion=%s ax=%.2f ay=%.2f az=%.2f",
             emotionToName(DeskRobo_GetEmotion()), Accel.x, Accel.y, Accel.z);
    server.send(200, "text/plain", buf);
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
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.softAP(kApSsid, kApPass);
  Serial.printf("[WEB] AP started: %s IP=%s\n", kApSsid, WiFi.softAPIP().toString().c_str());

  handleRoutes();
  server.begin();
  Serial.println("[WEB] HTTP server started");
}

void DeskRoboWeb_Loop() {
  server.handleClient();
}

