# DeskRobo (ESP32-S3-LCD-1.85)

DeskRobo is a small desk companion robot with an animated LVGL face on the Waveshare ESP32-S3-LCD-1.85.

Current MVP focus:
- stable display output
- animated glow eyes with emotions
- local control via Wi-Fi AP + web UI
- OTA firmware upload from browser

## Hardware

- Board: Waveshare ESP32-S3-LCD-1.85
- Display: ST77916 (360x360)
- Framework: Arduino (PlatformIO)
- Upload port (current setup): `COM5`

## Current Status

Working now:
- dark background + animated eyes
- blink and idle movement
- 16 emotions
- web control page in AP mode
- HTTP API for emotion/event triggers
- OTA upload endpoint

Temporarily disabled:
- gyro-based live reactions (kept off for stability)

## Emotion Set

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

## Quick Start

### 1) Build + Upload

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload
```

### 2) Connect to DeskRobo AP

- SSID: `DeskRobo-Setup`
- Password: `deskrobo123`
- Web UI: `http://192.168.4.1/`

## HTTP API

### Status

```http
GET /api/status
```

Example:

```text
emotion=IDLE ax=0.00 ay=0.00 az=0.00
```

### Set emotion

```http
POST /api/emotion?name=HAPPY&hold=3500
```

### Trigger event

```http
POST /api/event?name=CALL
```

Supported event names:
- `QUIET`
- `LOUD`
- `VERY_LOUD`
- `TILT`
- `SHAKE`
- `CALL`
- `MAIL`
- `TEAMS`

Note: `TILT`/`SHAKE` API exists, but gyro event handling is currently disabled in stable mode.

### OTA upload

```http
POST /api/ota
```

Use the web page file upload control to send a firmware `.bin`.

## Project Structure

- `src/main.cpp` boot/init and main loop
- `src/anime_face.*` glow-eye renderer
- `src/DeskRoboMVP.*` emotion/event state logic
- `src/DeskRoboWeb.*` AP web server + API + OTA
- `src/LVGL_Arduino/*` Waveshare display + board support
- `DESKROBO_QUICKSTART.md` detailed local quickstart

## Stability Notes

- This repository keeps a stable MVP baseline.
- Gyro is intentionally off in `platformio.ini` (`DESKROBO_ENABLE_GYRO`, `DESKROBO_GYRO_EVENTS` commented) due to prior reboot/blink loops.

## Roadmap

- re-enable gyro safely with robust init + failover
- Windows companion service (Teams/Outlook/audio -> DeskRobo events)
- SD-card based assets/themes
- richer face transitions and animation presets

## License

No license file is set yet.
Add a `LICENSE` file before publishing publicly.
