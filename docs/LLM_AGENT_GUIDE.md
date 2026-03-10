# DeskRobo LLM Agent Guide

This document is for coding agents (LLM/KI) that modify this repository.

## 1. Primary Goal

Keep firmware stable first. Prefer minimal, reversible changes.  
Do not break display bring-up, LVGL loop, or BLE command handling.

## 2. Critical Invariants

- `main.cpp` init order must stay valid: power -> I2C/EXIO -> backlight -> LCD -> LVGL -> app modules.
- UI/LVGL operations must run from main loop context.
- BLE callbacks must enqueue commands; do not directly mutate LVGL objects in BLE context.
- Keep `platformio.ini` stable unless user explicitly requests environment migration.

## 3. Where to Change What

- Face rendering/styling:
  - `src/anime_face.h`
  - `src/anime_face.cpp`

- App logic (emotion state, style/tuning persistence):
  - `src/DeskRoboMVP.h`
  - `src/DeskRoboMVP.cpp`

- HTTP API + web frontend:
  - `src/DeskRoboWeb.cpp`

- BLE command protocol:
  - `src/DeskRoboBLE.h`
  - `src/DeskRoboBLE.cpp`

- Low-level display/backlight:
  - `src/LVGL_Arduino/Display_ST77916.*`

- PC-side BLE/event bridge:
  - `pc_agent/*`

## 4. Safe Change Procedure

1. Inspect current files and API contracts (`rg` search first).
2. Edit only necessary files.
3. Build immediately.
4. If build passes, upload and validate runtime behavior.
5. Report exact modified files and functional impact.

## 5. Required Validation Commands (Windows)

Build:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
```

Upload:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload
```

Monitor:

```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor --port COM5 --baud 115200
```

## 6. API Contract Summary

- `POST /api/emotion?name=<EMOTION>&hold=<ms>`
- `POST /api/eyes?left=<EMOTION>&right=<EMOTION>&hold=<ms>`
- `POST /api/style?name=EVE|WALLE|CLASSIC`
- `POST /api/backlight?value=0..100`
- `POST /api/tune/set?key=<k>&value=<v>`
- `POST /api/event?name=CALL|MAIL|TEAMS|LOUD|VERY_LOUD|QUIET|TILT|SHAKE`
- `POST /api/ota` multipart upload

If adding endpoints, update:
- embedded web UI in `DeskRoboWeb.cpp`
- this document and `README.md`

## 7. Common Failure Modes

- Display blinks in test colors:
  - suspect low-level display path or unsafe thread interaction
  - verify no direct LVGL calls from BLE/task callbacks

- BLE says "sent" but no visible change:
  - verify queue/dispatch in `DeskRoboBLE_Loop()`
  - validate emotion code mapping and hold time

- Windows Bleak `pythoncom`/MTA issues in `pc_agent`:
  - ensure STA/MTA workaround remains active
  - avoid importing COM-heavy modules into scanner loop without care

## 8. Persistence Rules

- Style/tuning persistence uses Preferences namespace `deskrobo`.
- If you add persistent settings:
  - add save key in `DeskRobo_SaveTuning()`
  - add load key in `DeskRobo_LoadTuning()`
  - keep defaults backward compatible

## 9. Scope Discipline

- Do not migrate Arduino -> ESP-IDF unless explicitly requested.
- Do not perform broad refactors while fixing a specific issue.
- Keep ASCII-only docs/code unless file already uses Unicode.

## 10. Handoff Format (for agents)

When done, report:
1. modified files
2. behavior changes
3. build/upload result
4. any residual risks
