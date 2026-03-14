# MyDeskRobo Technical Documentation

## 1. Overview

Current public runtime:
- firmware target: `esp32-s3-mydeskrobo-full`
- renderer: `MyDeskRoboEngine`
- control path: BLE
- companion app: Windows PC agent

The current release is BLE-only in normal operation.
Old Wi-Fi/web documentation is not part of the active runtime path.

## 2. Runtime Architecture

Main loop responsibilities:
- display and LVGL bring-up
- MyDeskRobo runtime state machine
- BLE command loop
- local gyro motion loop
- LVGL flush loop

Key modules:
- `src/DeskRoboMVP_nextgen.cpp`
  - runtime state, active emotion, idle rotation, persistence, sleep logic
- `MyDeskRoboEngine/include/scenes/*.h`
  - scene definitions
- `MyDeskRoboEngine/src/LayerRenderer.cpp`
  - canvas-based renderer for accurate cuts and brows
- `src/DeskRoboBLE.cpp`
  - BLE parser and queued command handoff
- `src/main.cpp`
  - hardware bring-up, gyro loop, BLE startup
- `pc_agent/*`
  - Windows control app and BLE transport

## 3. Face Model

Style:
- `EVE` only

Public emotions:
- `IDLE`
- `HAPPY`
- `SAD`
- `ANGRY`
- `ANGRY_SOFT`
- `ANGRY_HARD`
- `WOW`
- `SLEEPY`
- `CONFUSED`
- `MAIL`
- `CALL`
- `SHAKE`
- `WINK`
- `XX`
- `GLITCH`

Eye-pair override exists and is controlled via BLE.

## 4. Event Logic

Events map into temporary emotions with priority and TTL.
Examples:
- `CALL` -> call scene
- `MAIL` -> confused/mail-like reaction
- `LOUD` -> wow
- `VERY_LOUD` -> shake
- `SHAKE` -> shake

Local gyro runtime currently generates only `SHAKE` for stability.
Local `TILT` generation has been disabled in the active runtime path.

## 5. BLE Control Path

BLE does not change LVGL directly.
BLE callbacks only enqueue commands for the main loop.

Supported command families:
- emotion
- eye pair
- event
- backlight
- status label
- tuning
- save/load tuning
- time sync
- factory reset

## 6. Persistence

Preferences namespace:
- `deskrobo`

Persisted values include:
- eye motion tuning
- blink and glow tuning
- shake tuning
- sleep/display timing
- eye color
- gyro thresholds

## 7. Boot and Power Behavior

Boot sequence:
- hardware power hold
- I2C / expander / battery / SD init
- gyro init
- backlight init
- LCD init
- LVGL init
- MyDeskRobo runtime init
- BLE task start

Boot UI:
- splash screen
- `My Robo Desk`
- `v0.5`

## 8. Build and Flash

Build:
```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Upload:
```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

Erase NVS / flash:
```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t erase
```

## 9. Stability Notes

- The canvas renderer is required for correct cut/brow rendering.
- Old saved preferences can override new firmware defaults.
- If behavior looks wrong after flashing, erase or factory-reset the device first.
- Gyro thresholds must stay conservative; false shake events are worse than missing weak taps.
