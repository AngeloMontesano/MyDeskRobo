# MyDeskRobo LLM Agent Guide

The public release target is the MyDeskRoboEngine-based EVE runtime.

## 1. Primary Goal

Keep `esp32-s3-mydeskrobo-full` stable.
Do not regress:
- display bring-up
- LVGL main-loop ownership
- BLE queue handoff
- web frontend control path

## 2. Critical Invariants

- LVGL work stays in main loop context.
- BLE callbacks only enqueue commands.
- Eye rendering goes through the canvas renderer, not ad-hoc rotated LVGL object tricks.
- Public runtime is EVE-only unless the user explicitly requests reopening style families.

## 3. Main Files

- Runtime state / API bridge:
  - `src/DeskRoboMVP.h`
  - `src/DeskRoboMVP_nextgen.cpp`
- Web API + embedded frontend:
  - `src/DeskRoboWeb.cpp`
- BLE control path:
  - `src/DeskRoboBLE.cpp`
- Scene engine:
  - `MyDeskRoboEngine/include/SceneSpec.h`
  - `MyDeskRoboEngine/include/LayerRenderer.h`
  - `MyDeskRoboEngine/src/LayerRenderer.cpp`
- Scene assets:
  - `MyDeskRoboEngine/include/scenes/*.h`
- Eye editor docs:
  - `docs/eye_designer/*`
- PC agent:
  - `pc_agent/*`

## 4. Safe Change Procedure

1. Search first.
2. Touch the minimum number of files.
3. Build the exact target you changed.
4. If Python GUI/agent code changed, run `py_compile`.
5. Report concrete file changes and residual risk.

## 5. Required Validation

Firmware build:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Firmware upload:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

Python check:

```powershell
python -m py_compile pc_agent\pc_agent.py pc_agent\agent_gui.py pc_agent\ble_client.py
```

## 6. API Summary

- `POST /api/emotion?name=<EMOTION>&hold=<ms>`
- `POST /api/eyes?left=<EMOTION>&right=<EMOTION>&hold=<ms>`
- `POST /api/style?name=EVE`
- `POST /api/backlight?value=0..100`
- `POST /api/tune/set?key=<k>&value=<v>`
- `POST /api/event?name=CALL|MAIL|TEAMS|LOUD|VERY_LOUD|QUIET|TILT|SHAKE`
- `POST /api/ota`

## 7. Common Failure Modes

- Eyes render unlike the editor:
  - inspect the canvas renderer first
  - do not reintroduce rotated LVGL object layering for `cut`/`brow`
- BLE says sent but nothing changes:
  - inspect `DeskRoboBLE.cpp` queueing and `DeskRoboMVP_nextgen.cpp`
- Web UI shows stale options:
  - check embedded HTML/JS in `DeskRoboWeb.cpp`
- GUI and firmware diverge:
  - align `pc_agent/agent_gui.py` with the public API, not just scene-demo assets

## 8. Persistence Rules

Preferences namespace: `deskrobo`

If new persistent settings are added:
- save in `DeskRobo_SaveTuning()`
- load in `DeskRobo_LoadTuning()` / `load_prefs_values()`
- keep backward-compatible defaults

## 9. Scope Discipline

- Do not silently widen styles again.
- Keep scene authoring inside `MyDeskRoboEngine`; do not reintroduce the removed legacy renderer path.
- Prefer scene/data changes over hardcoded emotion-specific drawing hacks.

## 10. Handoff Format

Report:
1. modified files
2. behavior changes
3. validation run
4. residual risks
