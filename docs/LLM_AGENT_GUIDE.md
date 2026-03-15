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
- PC agent loads emotion list dynamically via `LIST_EMOTIONS` BLE characteristic on connect

## 3. Critical Invariants

- LVGL work stays in the main loop.
- BLE callbacks enqueue commands only — never touch LVGL directly.
- Face rendering goes through the canvas renderer (`LayerRenderer`).
- Public runtime is EVE-only.
- `output_opa == 0` in LayerRenderer means COVER (LVGL fallback) — clamp fade values to min 1.
- If a change affects Python control code, run `py_compile`.

## 4. Main Files

Runtime:
- `src/DeskRoboMVP.h` — emotion/event enums, public API
- `src/DeskRoboMVP_nextgen.cpp` — emotion engine, render loop, WINK animation, fade system
- `src/main.cpp` — hardware init, gyro burst-detection loop
- `src/DeskRoboBLE.cpp` — BLE parser, LIST_EMOTIONS, SET_EMOTION

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

## 5. Emotion System

Active emotions (BLE names):
`IDLE, HAPPY, SAD, ANGRY, ANGRY_SOFT, ANGRY_HARD, WOW, SLEEPY, CONFUSED, DIZZY, MAIL, CALL, SHAKE, WINK, XX, GLITCH, BORED, FOCUSED, HAPPY_TONGUE, HOLLOW`

Removed (do not reintroduce): `SKEPTICAL`, `EXCITED`, `SUNGLASSES_AVIATOR`

Scene files: `MyDeskRoboEngine/include/scenes/eve_<name>.h`

Special cases:
- **WINK**: no fade-in, right eye animates close→hold→open via `output_opa`, left eye unchanged
- **DIZZY**: X-eyes via two crossing DrawCut RoundedRect at ±42°
- **HOLLOW**: DrawCut Ellipse removes iris interior → glowing ring
- **HAPPY_TONGUE**: reuses HAPPY ops + shows `g_tongue_obj` LVGL widget
- **ANGRY_HARD**: no pupils, hard brow cut only
- **FOCUSED**: `mirror_x_for_right=false` on pupil op → both eyes scan same direction

## 6. RenderOp Key Fields

```cpp
bool mirror_x_for_right;  // true → pupils converge; false → scan together
bool is_pupil;            // true → offset shifts by RuntimeState.pupil_x/y
LayerOp op;               // DrawGlow/Mid/Core = additive layers; DrawCut = subtracts
```

`side_x()` in LayerRenderer:
```cpp
pupil_off = (right && op.mirror_x_for_right) ? -state.pupil_x : state.pupil_x;
```

## 7. Gyro Motion Detection

Located in `src/main.cpp`, `GyroMotionLoop()`.
Requires `DESKROBO_ENABLE_GYRO=1` build flag (default 0).

Burst-hit classification:
- `g_burst_hits <= 6` → TAP → cycles idle round emotion
- `g_burst_hits > 6` + burst >= 500 ms → SHAKE → SHAKE emotion

Serial log format: `[GYRO] burst=Xms hits=Y → TAP/SHAKE/ignore`

## 8. Required Validation

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

## 9. Common Failure Modes

- **IDLE looks wrong but manual emotions work**: inspect reset/default path and boot transition logic.
- **Face differs from editor intent**: inspect canvas renderer and scene data; do not reintroduce rotated LVGL object layering.
- **Tuning command ignored**: inspect `DeskRoboBLE.cpp` tune-key parser.
- **Strange post-update behavior**: check persisted values in namespace `deskrobo`; erase if needed.
- **Tap detected as shake**: check `g_burst_hits` threshold (currently <=6 = TAP); log `hits=Y` shows actual value.

## 10. Scope Discipline

- Do not silently reintroduce web as the primary control path.
- Do not widen styles again unless explicitly requested.
- Prefer scene/data changes over ad-hoc rendering hacks.
- Keep release docs aligned with actual runtime behavior.
- When a feature consistently fails, delete it rather than keep patching.
