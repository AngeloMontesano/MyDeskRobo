# Release Notes

## MyDeskRobo v0.5

Current firmware target:
- `esp32-s3-mydeskrobo-full`

Current branding:
- boot splash: `My Robo Desk`
- version shown on boot: `v0.5`

## Included in This Release

- `MyDeskRoboEngine` scene renderer
- EVE-only face pipeline
- BLE control path
- Windows PC agent and GUI
- runtime eye tuning
- eye color tuning
- sleep/screensaver timing
- local gyro-based shake trigger
- special emotions `WINK`, `XX`, `GLITCH`

## Not Included in This Release

- Wi-Fi setup flow
- web frontend
- OTA over browser
- legacy `anime_face` renderer
- old style families like `FLUX`
- old public `ANGRY_1..10` / `SAD_1..10` variants

## Validation

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

## Release Notes for Users

Before flashing:
- verify the COM port in `platformio.ini`
- optionally erase flash first if you want a clean start

After flashing:
- the face should appear after the boot splash
- BLE should be available as `MyDeskRobo`
- the PC GUI should connect and control the device without Wi-Fi

## Before Public Publishing

Still recommended:
- add a real `LICENSE` file
