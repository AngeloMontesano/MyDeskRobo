# DeskRobo PC Agent

Windows-Agent, der lokale Ereignisse erkennt und Emotionen per BLE an DeskRobo sendet.

## Schnellstart (empfohlen)

1. `pc_agent` einmal einrichten:

```powershell
cd pc_agent
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

2. GUI per Doppelklick starten:

- `start_pc_agent.bat` (im Projekt-Root)
- oder `pc_agent\start_agent_gui.bat`

Die GUI zeigt klar an: `Suche`, `Verbinde`, `Verbunden`, `Neuversuch`, `Fehler`.

## Konsole (Fallback)

```powershell
cd pc_agent
.\.venv\Scripts\python.exe pc_agent.py --mode basic
```

Hinweis fuer altes PowerShell ohne `&&`: nutze `;` oder zwei Zeilen.

## Optional: Standalone EXE bauen

```powershell
cd pc_agent
.\build_gui_exe.bat
```

Ergebnis: `pc_agent\dist\DeskRoboAgent.exe`

## Modi

- `basic`: Outlook + Kalender
- `all`: alle Monitore
- `teams`: nur Teams zus. zu basic
- `mic`: nur Mikrofon zus. zu basic

## Dateistruktur

- `pc_agent.py` Hauptlogik + Dispatch
- `ble_client.py` BLE Connect/Reconnect + Protokoll
- `agent_gui.py` einfache Desktop-Oberflaeche
- `config.py` Einstellungen
- `start_agent_gui.bat` GUI-Doppelklickstart
- `start_agent_console.bat` Konsolenstart
- `build_gui_exe.bat` EXE-Build

## Emotion Bytes

- `0` IDLE
- `1` HAPPY
- `2` SAD
- `3` ANGRY
- `4` SURPRISED
- `5` SLEEPY
- `6` WINK
- `7` LOVE
- `10` CALL
- `11` EMAIL
- `12` TEAMS_MSG
- `13` CALENDAR
- `14` NOTIFY
- `15` LOCK
- `16` WIFI
