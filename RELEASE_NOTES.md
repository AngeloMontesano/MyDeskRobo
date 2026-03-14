# Release Notes

## Current Public Release

Project name:
- `MyDeskRobo`

Main firmware target:
- `esp32-s3-mydeskrobo-full`

Renderer:
- `MyDeskRoboEngine`
- EVE-only scene pipeline
- canvas-based eye rendering for accurate `cut` and `brow` layers

## Included Features

- Web frontend over Wi-Fi AP (`MyDeskRobo-Setup`)
- BLE control path
- Windows PC agent and GUI
- OTA upload from the browser
- Backlight control
- Idle tuning and persistence
- Sleep / display-off timing
- Runtime eye color tuning
- EVE emotions including `WINK`, `XX`, `GLITCH`

## Removed During Release Cleanup

- legacy `anime_face` renderer path
- old `DeskRoboMVP.cpp` runtime path
- old multi-style families (`FLUX`, `ANGRY`, `SAD`)
- old `ANGRY_1..10` / `SAD_1..10` public variants
- demo target and demo-only files
- temporary preview / roboeyes experiment files

## Build Validation

Firmware:
```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Python:
```powershell
python -m py_compile pc_agent\pc_agent.py pc_agent\agent_gui.py pc_agent\ble_client.py
```

## Before Publishing

- add a real `LICENSE` file
- optionally rename the outer repo folder itself to `MyDeskRobo`
- commit the rename/delete set as one coherent cleanup change
