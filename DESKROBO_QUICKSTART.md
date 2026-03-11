# DeskRobo Quickstart (Current Stable MVP)

This document matches the current working firmware state in this project.

## 1) Build and Flash

- Environment: `platformio.ini` default env
- Upload port: `COM5`

PowerShell commands:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload
```

## 2) Display Result

After boot, the display should show:

- dark background
- animated glow eyes
- blinking and idle movement

Gyro reactions are currently disabled for stability.

## 3) AP + Web Control

The device starts its own Wi-Fi AP:

- SSID: `DeskRobo-Setup`
- Password: `deskrobo123`
- Device IP: usually `192.168.4.1`

Open in browser:

- `http://192.168.4.1/`

## 4) HTTP API

### Status

```http
GET /api/status
```

Example response:

```text
emotion=IDLE ax=0.00 ay=0.00 az=0.00
```

### Set emotion

```http
POST /api/emotion?name=HAPPY&hold=3500
```

Supported `name`:

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
- `LOCKED`
- `WIFI`

### Trigger event

```http
POST /api/event?name=CALL
```

Supported `name`:

- `QUIET`
- `LOUD`
- `VERY_LOUD`
- `TILT`
- `SHAKE`
- `CALL`
- `MAIL`
- `TEAMS`

Note: `TILT`/`SHAKE` endpoints exist, but real gyro logic is currently disabled in stable mode.

## 5) OTA Update via Browser

In web UI:

1. Connect to AP `DeskRobo-Setup`
2. Open `http://192.168.4.1/`
3. Select firmware `.bin`
4. Click `Upload Firmware`

The device reboots automatically on successful OTA.

## 6) Serial Monitor (optional)

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM5 --baud 115200
```

Expected boot lines include:

- `[WEB] AP started: DeskRobo-Setup IP=...`
- `[WEB] HTTP server started`

## 7) Current Stability Flags

In `platformio.ini`:

- `DESKROBO_ENABLE_GYRO` is currently disabled (commented out)
- `DESKROBO_GYRO_EVENTS` is currently disabled (commented out)

This is intentional because previous gyro test paths caused reboot loops and display blinking.
