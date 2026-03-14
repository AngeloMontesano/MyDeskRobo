# MyDeskRobo PC Agent

The PC agent is the main user-facing control app for the current release.
It connects to MyDeskRobo over BLE and can:
- trigger emotions
- send events
- change eye color
- change blink and motion tuning
- change sleep/display timing
- factory reset the device

## 1. One-Time Setup

```powershell
cd pc_agent
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

## 2. Start the GUI

Preferred:
- `start_pc_agent.bat`
- or `pc_agent\start_agent_gui.bat`

The GUI starts the agent automatically in mode `all`.

## 3. Modes

- `all`: all monitors enabled
- `basic`: Outlook + calendar
- `teams`: adds Teams monitoring
- `mic`: adds microphone monitoring

## 4. Main GUI Functions

Quick control:
- show an emotion now
- set left/right eye separately
- adjust brightness
- trigger test events like `CALL`, `MAIL`, `SHAKE`

Behavior and display:
- set blink and motion values
- set eye color
- set screensaver and display-off timing
- save values on the device
- load values from the device
- reset the device to factory defaults

## 5. Console Fallback

```powershell
cd pc_agent
.\.venv\Scripts\python.exe pc_agent.py --mode all
```

## 6. Direct BLE Control Examples

```powershell
# show one emotion
.\.venv\Scripts\python.exe pc_agent.py --emotion-name HAPPY --emotion-hold 4000

# set left and right eye separately
.\.venv\Scripts\python.exe pc_agent.py --eyes HAPPY:WINK:5000

# set brightness
.\.venv\Scripts\python.exe pc_agent.py --backlight 70

# send one event
.\.venv\Scripts\python.exe pc_agent.py --event CALL

# set tuning values
.\.venv\Scripts\python.exe pc_agent.py --tune drift_amp_px=2 --tune blink_interval_ms=3600
```

## 7. Troubleshooting

If the GUI cannot find the device:
- make sure MyDeskRobo is powered
- wait until the boot splash is gone
- check that Windows Bluetooth is enabled
- stop other programs that might hold the BLE connection

If the face behaves strangely after an update:
- use `Werkseinstellungen` in the GUI
- then reconnect after the device reboots
