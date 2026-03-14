# MyDeskRobo Quickstart

This quickstart describes the current public runtime target.

## 1. Build and Flash

Target:
- `esp32-s3-mydeskrobo-full`

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

## 2. First Boot

MyDeskRobo waits up to 60 seconds for configured STA Wi-Fi.

If Wi-Fi connects in time:
- Wi-Fi stays active
- BLE starts

If Wi-Fi does not connect in time:
- AP/STA Wi-Fi is turned off
- BLE starts

During setup/AP window:
- SSID: `MyDeskRobo-Setup`
- Password: `deskrobo123`
- URL: `http://192.168.4.1/`

## 3. Web Frontend

The embedded frontend provides:
- emotion buttons
- eye pair override
- EVE style apply
- backlight control
- Wi-Fi setup
- audio test
- debug status label toggle
- idle tuning save/load
- OTA update

## 4. Main HTTP Calls

- `GET /api/status`
- `POST /api/emotion?name=HAPPY&hold=3500`
- `POST /api/eyes?left=IDLE&right=WINK&hold=5000`
- `POST /api/style?name=EVE`
- `POST /api/backlight?value=65`
- `POST /api/event?name=CALL`
- `POST /api/tune/save`
- `POST /api/ota`

## 5. PC Agent

Setup:

```powershell
cd pc_agent
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

GUI:
- `start_pc_agent.bat`
- or `pc_agent\start_agent_gui.bat`

Console fallback:

```powershell
cd pc_agent
.\.venv\Scripts\python.exe pc_agent.py --mode basic
```

## 6. Validation

Firmware build:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Python check:

```powershell
python -m py_compile pc_agent\pc_agent.py pc_agent\agent_gui.py pc_agent\ble_client.py
```

## 7. Release Note

Add a real `LICENSE` file before publishing the repository publicly.
