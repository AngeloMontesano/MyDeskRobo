# DeskRobo (Waveshare ESP32-S3-LCD-1.85)

DeskRobo is an ESP32 desk companion with an animated LVGL face, controllable over Web API and BLE.

This repository contains:
- firmware (`src/*`) for the board
- Windows companion agent (`pc_agent/*`) for BLE event forwarding

## Features

- 360x360 display (ST77916) with animated eye face
- 18 emotions and temporary eye pair override
- selectable face styles: `EVE`, `PLAYFUL`
- web control UI in AP mode (`DeskRobo-Setup`)
- HTTP API for emotions, events, style, backlight, tuning
- OTA firmware upload via browser (`/api/ota`)
- BLE command path (for PC agent)
- backlight control from web UI (`0..100`)

## Connectivity Policy

- On boot, DeskRobo waits up to 60 seconds for STA WLAN connection.
- If WLAN connects within 60s: BLE starts and Wi-Fi stays active.
- If WLAN does not connect within 60s: Wi-Fi (AP+STA) is turned off and BLE starts.
- AP auto-off is still active: AP is disabled after 9 minutes when no AP clients are connected.

## Hardware / Software Baseline

- Board: Waveshare ESP32-S3-LCD-1.85
- Framework: Arduino (PlatformIO)
- Current upload/monitor port in this repo: `COM5`
- OS used for development: Windows

## Quick Start

1. Build + upload:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload
```

2. Connect to AP (within the first 60s after boot if no STA WLAN is connected):
- SSID: `DeskRobo-Setup`
- Password: `deskrobo123`

3. Open:
- `http://192.168.4.1/`

Note: if Wi-Fi timed out and was turned off, reboot DeskRobo to reopen the setup AP window.

## Main API Endpoints

- `GET /api/status`
- `POST /api/emotion?name=HAPPY&hold=3500`
- `POST /api/eyes?left=IDLE&right=WINK&hold=5000`
- `POST /api/style?name=EVE|PLAYFUL`
- `GET /api/backlight`
- `POST /api/backlight?value=65`
- `GET /api/tune/get`
- `POST /api/tune/set?key=blink_interval_ms&value=3200`
- `POST /api/tune/save`
- `POST /api/tune/load`
- `POST /api/event?name=CALL`
- `POST /api/ota`

## Documentation

- User/developer quickstart: [DESKROBO_QUICKSTART.md](DESKROBO_QUICKSTART.md)
- Technical documentation: [docs/TECHNICAL_DOC.md](docs/TECHNICAL_DOC.md)
- LLM/Agent implementation guide: [docs/LLM_AGENT_GUIDE.md](docs/LLM_AGENT_GUIDE.md)
- PC agent details: [pc_agent/README.md](pc_agent/README.md)

## Known Constraints

- Gyro runtime events are disabled by default for stability (`DESKROBO_ENABLE_GYRO`, `DESKROBO_GYRO_EVENTS` commented in `platformio.ini`).
- Some LVGL compile warnings from upstream headers are expected and currently non-blocking.

## License

No license file is set yet. Add a `LICENSE` file before public release.
