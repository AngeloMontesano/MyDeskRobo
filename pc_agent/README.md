# MyDeskRobo PC Agent

Windows-Agent, der lokale Ereignisse erkennt und Emotionen per BLE an MyDeskRobo sendet.

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

Ergebnis: `pc_agent\dist\MyDeskRoboAgent.exe`

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

## BLE Zeitsync

- Beim Verbinden sendet der Agent automatisch die aktuelle Unix-Zeit an MyDeskRobo (TIME:<seq>:<epoch>).
- Danach erfolgt periodische Nachsynchronisierung (Standard: alle 10 Minuten).
- Bei Fehler wird nach 30 Sekunden erneut versucht (konfigurierbar in config.py).

## BLE Remote Control (wie Webfrontend)

Mit CLI-Flags kannst du zentrale Webfrontend-Funktionen direkt ueber BLE senden:

```powershell
cd pc_agent
.\.venv\Scripts\python.exe pc_agent.py --style EVE --backlight 70 --status-label on
```

Weitere Beispiele:

```powershell
# Event ausloesen
.\.venv\Scripts\python.exe pc_agent.py --event CALL

# Emotion + Hold
.\.venv\Scripts\python.exe pc_agent.py --emotion-name HAPPY --emotion-hold 4000

# Linkes/Rechtes Auge setzen (inkl. Hold in ms)
.\.venv\Scripts\python.exe pc_agent.py --eyes HAPPY:ANGRY:5000

# Idle-Tuning setzen (mehrfach moeglich)
.\.venv\Scripts\python.exe pc_agent.py --tune drift_amp_px=2 --tune blink_interval_ms=3500

# Rohbefehl senden (CMD-Payload)
.\.venv\Scripts\python.exe pc_agent.py --cmd "STYLE:EVE_CINEMATIC"
```

