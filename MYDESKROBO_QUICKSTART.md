# MyDeskRobo Quickstart

This is the practical flashing guide for the current release.
It assumes Windows and the Waveshare ESP32-S3-LCD-1.85.

## 1. Requirements

Hardware:
- Waveshare ESP32-S3-LCD-1.85
- USB data cable
- Windows PC with Bluetooth

Software:
- VS Code with PlatformIO
- or PlatformIO Core installed locally
- Python 3.11+ recommended for the PC agent

## 2. Get the Project

Clone or download the repository, then open it in VS Code / PlatformIO.

Project root:
```text
MyDeskRobo
```

## 3. Check the COM Port

The repo currently defaults to `COM5` in [platformio.ini](platformio.ini).
If your device appears on another port, change these lines:

```ini
upload_port = COM5
monitor_port = COM5
```

Set them to your real port, for example `COM3` or `COM7`.

## 4. Optional: Erase Old Settings First

Recommended before first public flash or after larger updates.
This clears stored tuning values from NVS.

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t erase
```

## 5. Build and Flash

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

## 6. Optional: Serial Monitor

Use this if you want boot logs or want to debug startup.

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor -b 115200 -p COM5
```

Adjust `COM5` if needed.

## 7. What You Should See After Boot

On the display:
- boot logo
- `My Robo Desk`
- `v0.5`
- then the EVE face

Expected runtime behavior:
- BLE starts automatically
- no Wi-Fi setup is required for the current release
- `IDLE` is the base state
- after inactivity, the idle rotation cycles through emotions automatically
- after about 10 minutes without BLE activity, sleepy/screensaver behavior begins
- after about 15 minutes, the display can turn off
- a short knock triggers the next idle emotion, a sustained shake plays the shake animation (requires `DESKROBO_ENABLE_GYRO=1` build flag)

## 8. PC Agent GUI

Setup once:

```powershell
cd pc_agent
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

Start GUI:
- double-click `start_pc_agent.bat`
- or run `pc_agent\start_agent_gui.bat`

What the GUI is for:
- send emotions over BLE
- run demo mode (cycles all emotions automatically)
- set left/right eye independently (eye-pair mode)
- set eye color
- change motion and blink tuning
- change sleep/display timers
- map PC events (mail, call, Teams) to emotions
- factory reset the device

## 9. If Flashing Fails

Check these points:
- correct USB data cable
- correct COM port in `platformio.ini`
- no serial monitor is still open on the same port
- board is powered and not stuck in another boot mode

## 10. If the Face Looks Wrong After Flashing

Do this in order:
1. use `Factory Reset` in the PC GUI
2. if needed, erase flash again with `-t erase`
3. flash again

This matters because old saved tuning values survive a normal firmware upload.

## 11. Minimal Validation

Firmware build:
```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Python validation:
```powershell
python -m py_compile pc_agent\pc_agent.py pc_agent\agent_gui.py pc_agent\ble_client.py pc_agent\dispatch.py
```
