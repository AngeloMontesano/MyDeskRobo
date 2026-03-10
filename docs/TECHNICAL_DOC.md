# DeskRobo Technical Documentation

## 1. Overview

DeskRobo firmware runs on a Waveshare ESP32-S3-LCD-1.85 using Arduino + PlatformIO.  
Core responsibilities:
- initialize board/display/LVGL
- render animated eyes with multiple styles/emotions
- expose local AP + web API for control and OTA
- accept BLE commands from PC agent

## 2. Runtime Architecture

## 2.1 Boot Sequence

`src/main.cpp` boot order:
1. power hold pin setup
2. I2C + EXIO init
3. optional gyro init (compile flag)
4. backlight init and default level
5. LCD init
6. LVGL init
7. DeskRobo MVP init
8. Web AP/server init
9. BLE init

Main loop:
- `DeskRobo_Loop()`
- `DeskRoboWeb_Loop()`
- `DeskRoboBLE_Loop()`
- `Lvgl_Loop()`

## 2.2 Modules

- `src/anime_face.*`  
  Eye renderer and animation timing (blink, drift, saccade, glow pulse), style presets.

- `src/DeskRoboMVP.*`  
  Emotion state machine, event priority/TTL, tuning persistence, style selection.

- `src/DeskRoboWeb.*`  
  Wi-Fi AP + HTTP server + embedded UI + OTA upload endpoint.

- `src/DeskRoboBLE.*`  
  BLE command interface and command queue handoff to main loop.

- `src/LVGL_Arduino/*`  
  Waveshare display/board support drivers.

## 3. Face Model

## 3.1 Emotions

Supported emotions:
- `IDLE`
- `HAPPY`
- `SAD`
- `ANGRY`
- `ANGST`
- `WOW`
- `SLEEPY`
- `LOVE`
- `CONFUSED`
- `EXCITED`
- `ANRUF`
- `LAUT`
- `MAIL`
- `DENKEN`
- `WINK`
- `GLITCH`

## 3.2 Styles

Supported styles:
- `EVE`
- `WALLE`
- `CLASSIC`

Style is applied in renderer (`AnimeFace::setStyle`) and persisted via Preferences through MVP save/load.

## 3.3 Eye Pair Override

`DeskRobo_SetEyePair(left, right, hold_ms)` forces left/right emotions for a temporary window; after timeout, normal emotion flow resumes.

## 4. Event and Priority Logic

`DeskRobo_PushEvent()` maps event to `(emotion, priority, ttl)` and suppresses lower-priority events while active TTL has not expired.

Examples:
- `CALL` -> high priority `ANRUF`
- `MAIL` -> medium priority `MAIL`
- loud audio -> `LAUT` / `WOW`

## 5. Web API

Base: AP mode (`DeskRobo-Setup`) at `http://192.168.4.1/`

- `GET /api/status`
- `POST /api/emotion?name=<EMOTION>&hold=<ms>`
- `POST /api/eyes?left=<EMOTION>&right=<EMOTION>&hold=<ms>`
- `POST /api/style?name=EVE|WALLE|CLASSIC`
- `GET /api/backlight`
- `POST /api/backlight?value=0..100`
- `GET /api/tune/get`
- `POST /api/tune/set?key=<k>&value=<v>`
- `POST /api/tune/save`
- `POST /api/tune/load`
- `POST /api/event?name=QUIET|LOUD|VERY_LOUD|TILT|SHAKE|CALL|MAIL|TEAMS`
- `POST /api/ota` (multipart firmware upload)

## 6. BLE Control Path

BLE commands are received in BLE context and queued to main loop to avoid LVGL cross-thread access issues.

Accepted command families include:
- numeric emotion codes
- text commands such as `EVENT:*`, `EMOTION:*`, `EYES:*`

## 7. Persistence

Namespace: `Preferences("deskrobo")`

Persisted values:
- style (`face_style`)
- tuning parameters (`drift`, `saccade`, `blink`, `glow`)

Backlight is runtime adjustable via API; current level is tracked by `LCD_Backlight`.

## 8. Build and Flash

Using PlatformIO environment in `platformio.ini`:
- board: `esp32-s3-devkitc-1`
- framework: `arduino`
- upload/monitor port (current): `COM5`

Recommended command (Windows):

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload
```

## 9. Stability Notes

- Gyro event flags are disabled by default in `platformio.ini`.
- If display starts blinking test colors, suspect display driver path/regression and roll back recent low-level display changes first.

## 10. Extension Points

- Add new face styles in `anime_face.*` enum + draw logic.
- Add new web actions in `DeskRoboWeb.cpp` + MVP bridge.
- Add new BLE command types in `DeskRoboBLE.cpp`.
- Add new external event source in `pc_agent/monitors/*`.
