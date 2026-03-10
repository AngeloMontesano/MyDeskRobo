# DeskRobo PC Agent

Windows asyncio agent that watches local events and sends one-byte emotions to DeskRobo over BLE.

## Structure

- `pc_agent.py` main + dispatch loop
- `ble_client.py` BLE connect/reconnect + send
- `config.py` settings
- `monitors/teams.py`
- `monitors/outlook.py`
- `monitors/calendar_mon.py`
- `monitors/microphone.py`

## Install

```powershell
cd pc_agent
python -m venv .venv
.\.venv\Scripts\activate
pip install -r requirements.txt
```

## Run

Basic monitors (Outlook + Calendar):

```powershell
python pc_agent.py --mode basic
```

All monitors:

```powershell
python pc_agent.py --mode all
```

One-shot byte send:

```powershell
python pc_agent.py --send 11
```

## Emotion bytes

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

## Notes

- `pywin32` and `pycaw` monitors are Windows-only.
- `safe_monitor()` restarts failed monitor tasks after 10 seconds.
- On Ctrl+C, the agent sends `IDLE` and disconnects BLE.
