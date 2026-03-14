# MyDeskRobo

MyDeskRobo is an ESP32-S3 desk companion for the Waveshare ESP32-S3-LCD-1.85.
The current public release uses the new `MyDeskRoboEngine` renderer, BLE control and the Windows PC agent GUI.

## Current Release Scope

Included:
- firmware target `esp32-s3-mydeskrobo-full`
- EVE-based face pipeline
- BLE control path
- Windows PC agent and GUI
- boot splash: `My Robo Desk v0.5`
- runtime tuning for eye motion, eye color, sleep and display-off timing
- local shake detection via gyro


## Supported Emotions

- `IDLE`
- `HAPPY`
- `SAD`
- `ANGRY`
- `ANGRY_SOFT`
- `ANGRY_HARD`
- `WOW`
- `SLEEPY`
- `CONFUSED`
- `SHAKE`
- `WINK`
- `XX`
- `GLITCH`
- `MAIL`
- `CALL`

## For Users

Start here:
- [MYDESKROBO_QUICKSTART.md](MYDESKROBO_QUICKSTART.md)

That document explains:
- what hardware you need
- how to install PlatformIO
- how to set the COM port
- how to erase old settings
- how to flash the firmware
- how to start the PC agent GUI

## Repository Layout

- `src/` firmware entry points and runtime bridge
- `MyDeskRoboEngine/` face renderer and scene definitions
- `pc_agent/` Windows BLE agent and GUI
- `docs/` technical and maintenance documentation

## Validation

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

## Documentation

- [MYDESKROBO_QUICKSTART.md](MYDESKROBO_QUICKSTART.md)
- [docs/TECHNICAL_DOC.md](docs/TECHNICAL_DOC.md)
- [docs/LLM_AGENT_GUIDE.md](docs/LLM_AGENT_GUIDE.md)
- [pc_agent/README.md](pc_agent/README.md)
- [RELEASE_NOTES.md](RELEASE_NOTES.md)

## License

Add a `LICENSE` file before publishing publicly.
