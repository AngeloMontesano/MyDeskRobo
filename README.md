# MyDeskRobo

MyDeskRobo is an ESP32-S3 desk companion for the Waveshare ESP32-S3-LCD-1.85 (round display, 360×360, no touch).
The firmware renders animated eyes on the display and reacts to motion (IMU) and BLE commands from a Windows PC agent.

## Demo

![MyDeskRobo Demo](MyDeskRobo.gif)

## Origin

The 3D printed body is from **Deskimon** by CreativeChance:
[thangs.com — Deskimon: Cute Robots For Your Desk](https://thangs.com/designer/CreativeChance/post/Deskimon%20-%20Cute%20Robots%20For%20Your%20Desk-252349)

I didn't read the instructions properly and bought the wrong display.
So I scaled the body to 104% to fit the Waveshare ESP32-S3-LCD-1.85, wrote all the emotions and firmware myself (with AI assistance), and ended up with MyDeskRobo.

## Current Release Scope

Included:
- firmware target `esp32-s3-mydeskrobo-full`
- EVE face renderer (`MyDeskRoboEngine`) — canvas-based, single color per eye
- BLE control path (PC agent ↔ device)
- Windows PC agent for Outlook, Teams and co. Events 
- windows GUI for Finetune and with demo mode
- boot splash: `My Robo Desk v0.5`
- 20 emotions with animated transitions (fade-to-black between scenes)
- idle round-robin: spontaneous emotion changes after inactivity
- micro-expressions: random brief expressions during idle
- knock / shake detection via QMI8658 IMU (optional, requires build flag)
- runtime tuning for eye motion, eye color, blink, glow, sleep and display-off timing
- screensaver / display-off after configurable idle timeout

## Supported Emotions

| BLE name | Description |
|----------|-------------|
| `IDLE` | default resting face |
| `HAPPY` | happy eyes |
| `SAD` | sad eyes |
| `ANGRY` | hard angry (alias for ANGRY_HARD) |
| `ANGRY_SOFT` | soft angry |
| `ANGRY_HARD` | hard scowl, no pupils |
| `WOW` | wide surprised eyes |
| `SLEEPY` | drooping eyelids |
| `CONFUSED` | questioning look with ?? overlay |
| `DIZZY` | X-eyes (cartoon KO look) |
| `MAIL` | mail notification (alias for CONFUSED) |
| `CALL` | incoming call look |
| `SHAKE` | chaotic eye jitter |
| `WINK` | right eye winks (animated close → hold → open) |
| `XX` | defeated X eyes |
| `GLITCH` | glitch FX with red color flash |
| `BORED` | slow eye-roll upward |
| `FOCUSED` | pupils scan left→right together |
| `HAPPY_TONGUE` | happy + red tongue |
| `HOLLOW` | glowing ring iris |

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

- `src/` — firmware entry points and runtime bridge
- `MyDeskRoboEngine/` — face renderer and scene definitions
- `pc_agent/` — Windows BLE agent and GUI
- `docs/` — technical and maintenance documentation

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
python -m py_compile pc_agent\pc_agent.py pc_agent\agent_gui.py pc_agent\ble_client.py pc_agent\dispatch.py
```

## Documentation

- [MYDESKROBO_QUICKSTART.md](MYDESKROBO_QUICKSTART.md)
- [docs/TECHNICAL_DOC.md](docs/TECHNICAL_DOC.md)
- [docs/LLM_AGENT_GUIDE.md](docs/LLM_AGENT_GUIDE.md)
- [pc_agent/README.md](pc_agent/README.md)
- [RELEASE_NOTES.md](RELEASE_NOTES.md)

## License

This project is licensed under the MIT License.
See [LICENSE](LICENSE) for the full text.
