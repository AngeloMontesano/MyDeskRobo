# MyDeskRobo LLM Agent Guide

## 1. Primary Goal

Keep `esp32-s3-mydeskrobo-full` stable.
Do not regress:
- display bring-up
- BLE command handoff
- canvas-based face rendering
- PC-agent compatibility

## 2. Current Public Runtime

Active release path:
- `esp32-s3-mydeskrobo-full`
- `MyDeskRoboEngine`
- BLE-only control in normal use
- no active web frontend in the release workflow

## 3. Critical Invariants

- LVGL work stays in the main loop.
- BLE callbacks enqueue commands only.
- Face rendering goes through the canvas renderer.
- Public runtime is EVE-only.
- If a change affects Python control code, run `py_compile`.

## 4. Main Files

Runtime:
- `src/DeskRoboMVP.h`
- `src/DeskRoboMVP_nextgen.cpp`
- `src/main.cpp`
- `src/DeskRoboBLE.cpp`

Renderer:
- `MyDeskRoboEngine/include/SceneSpec.h`
- `MyDeskRoboEngine/include/LayerRenderer.h`
- `MyDeskRoboEngine/src/LayerRenderer.cpp`
- `MyDeskRoboEngine/include/scenes/*.h`

PC control:
- `pc_agent/agent_gui.py`
- `pc_agent/pc_agent.py`
- `pc_agent/ble_client.py`
- `pc_agent/dispatch.py`

## 5. Required Validation

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

## 6. Common Failure Modes

- `IDLE` looks wrong but manual emotions work:
  - inspect the reset/default path first
  - especially agent-triggered IDLE or boot transition logic
- face differs from editor intent:
  - inspect canvas renderer and scene data first
  - do not reintroduce rotated LVGL object layering
- tuning command appears ignored:
  - inspect `DeskRoboBLE.cpp` tune-key parser first
- strange post-update behavior:
  - check persisted values in namespace `deskrobo`

## 7. Scope Discipline

- Do not silently reintroduce web as the primary control path.
- Do not widen styles again unless explicitly requested.
- Prefer scene/data changes over ad-hoc rendering hacks.
- Keep release docs aligned with actual runtime behavior.
