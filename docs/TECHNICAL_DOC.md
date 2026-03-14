# MyDeskRobo Technical Documentation

## 1. Overview

MyDeskRobo runs on the Waveshare ESP32-S3-LCD-1.85 with Arduino + PlatformIO.

Current public-facing runtime path:
- `esp32-s3-mydeskrobo-full`
- MyDeskRoboEngine EVE-only scene renderer
- Web UI, BLE control path, OTA, backlight, tuning persistence

The public release path uses `MyDeskRoboEngine` and the EVE-only scene renderer.

## 2. Runtime Architecture

Main firmware loop:
- display/backlight/LVGL bring-up
- MyDeskRobo app runtime
- web server loop
- BLE command loop
- LVGL flush loop

Key modules:
- `src/DeskRoboMVP_nextgen.cpp`
  - app state, active emotion, event priority/TTL, tuning, persistence, sleep/display policy
- `MyDeskRoboEngine/include/scenes/*.h`
  - EVE scene definitions
- `MyDeskRoboEngine/src/LayerRenderer.cpp`
  - canvas-based renderer used by the public runtime
- `src/DeskRoboWeb.cpp`
  - AP mode, HTTP API, embedded frontend, OTA
- `src/DeskRoboBLE.cpp`
  - BLE protocol and queue handoff into main loop
- `pc_agent/*`
  - Windows BLE agent and GUI

## 3. Face Model

### 3.1 Style

Current release style:
- `EVE`

Old style families are intentionally not exposed in the new runtime.

### 3.2 Emotions

Public control path currently exposes:
- `IDLE`
- `HAPPY`
- `SAD`
- `ANGRY`
- `WOW`
- `SLEEPY`
- `CONFUSED`
- `MAIL`
- `CALL`
- `SHAKE`
- `WINK`
- `XX`
- `GLITCH`

### 3.3 Eye Pair Override

`DeskRobo_SetEyePair(left, right, hold_ms)` temporarily forces left/right eye scenes.

## 4. Event Logic

Events are mapped into temporary emotions with priority and TTL.

Examples:
- `CALL` -> call scene
- `MAIL` -> confused/mail-style reaction
- `LOUD` -> wow
- `VERY_LOUD` -> shake
- `TILT` -> confused
- `SHAKE` -> shake

## 5. Web API

While Wi-Fi is active, the device serves AP mode as `MyDeskRobo-Setup` at `http://192.168.4.1/`.

Main endpoints:
- `GET /api/status`
- `POST /api/emotion?name=<EMOTION>&hold=<ms>`
- `POST /api/eyes?left=<EMOTION>&right=<EMOTION>&hold=<ms>`
- `POST /api/style?name=EVE`
- `GET /api/backlight`
- `POST /api/backlight?value=0..100`
- `GET /api/tune/get`
- `POST /api/tune/set?key=<k>&value=<v>`
- `POST /api/tune/save`
- `POST /api/tune/load`
- `POST /api/event?name=QUIET|LOUD|VERY_LOUD|TILT|SHAKE|CALL|MAIL|TEAMS`
- `POST /api/ota`

## 6. BLE Control Path

BLE never mutates LVGL directly.
BLE callbacks parse commands and enqueue work for the main loop.

Accepted control families include:
- emotion
- event
- eye pair
- style
- backlight
- tuning
- time sync

## 7. Persistence

Preferences namespace:
- `deskrobo`

Persisted settings include:
- idle tuning values
- blink/glow/shake tuning
- sleep/display timing
- eye color
- gyro thresholds

The current runtime forces style persistence to `EVE`.

## 8. Build Targets

Main release target:
- `esp32-s3-mydeskrobo-full`


Example build:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Example upload:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

## 9. Stability Notes

- Gyro event behavior should be validated carefully; display stability has priority.
- The canvas renderer is required for accurate `cut`/`brow` rendering.
- Small overlay labels like `Zzz` and `?` remain more fragile than scene-native render ops.

## 10. Public Release Checklist

Before publishing:
- confirm `esp32-s3-mydeskrobo-full` builds cleanly
- confirm PC GUI Python files compile cleanly
- ensure docs match the nextgen path
- add a real `LICENSE` file
