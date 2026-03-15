---
name: project_architecture
description: Renderer architecture, key structs, and non-obvious design decisions
type: project
---

## LayerRenderer / RenderOp

Each eye is a separate LVGL canvas. `LayerRenderer::render_eye()` iterates `RenderOp` array and draws primitives.

**RenderOp fields:**
```cpp
bool enabled;
const char *name;
LayerOp op;           // DrawGlow, DrawMid, DrawCore, DrawCut, DrawShape, DrawBrow
Primitive primitive;  // Ellipse, RoundedRect
Side side;            // Left, Right, Both
bool mirror_x_for_right;   // negates pupil_x for right eye (convergence)
bool mirror_angle_for_right; // negates angle for right eye
lv_point_t offset;
lv_point_t size;
lv_coord_t radius;
int16_t angle;
ColorSpec color;
lv_opa_t opa;
bool is_pupil;        // if true, offset shifts by pupil_x/y from RuntimeState
```

**CRITICAL: `output_opa == 0` means COVER** (LVGL fallback). Always clamp to minimum 1 when fading.

**`side_x()` logic:**
```cpp
pupil_off = (right && op.mirror_x_for_right) ? -state.pupil_x : state.pupil_x;
```
- `mirror_x_for_right=true` → pupils converge (e.g. ANGRY_HARD was, now removed)
- `mirror_x_for_right=false` → both eyes scan same direction (FOCUSED)

## Fade-to-black transition

- `g_render_emotion` trails `g_current_emotion`
- When they differ and not in eye-pair mode: fade starts
- Phase 1 (0..80ms): output_opa 255→1 (fade out old scene)
- Phase 2 (80..160ms): output_opa 1→255 (fade in new scene), `g_render_emotion` updated at midpoint
- `kFadeHalfMs = 80`

## WINK animation system

- `g_wink_start_ms`: timestamp when wink animation started (0 = inactive)
- Set immediately in `DeskRobo_SetEmotion(WINK)` and idle round-robin
- `g_render_emotion` also set immediately (skips fade-in)
- In `render_active()`: when `right_emotion == WINK && g_wink_start_ms != 0`, overrides `right_state.output_opa`
- Durations: `kWinkCloseMs=80, kWinkHoldMs=70, kWinkOpenMs=150, kWinkTotalMs=300`

## Eye pair mode

`DeskRobo_SetEyePair(left, right, hold_ms)` — left and right canvas show different scenes.
- Disables fade logic entirely
- Used for split expressions

## Motion / gyro (main.cpp)

- `DESKROBO_ENABLE_GYRO` must be 1 in build flags (default 0)
- Poll interval: 40ms
- Burst detection: `g_burst_start_ms`, `g_burst_hits`, `g_calm_streak`
- TAP: `hits <= 6`, cooldown > 600ms → `DESKROBO_EVENT_MOTION_TAP` → cycles idle round
- SHAKE: `hits > 6`, burst >= 500ms, cooldown > 2600ms → `DESKROBO_EVENT_MOTION_SHAKE`
- Serial log: `[GYRO] burst=Xms hits=Y → TAP/SHAKE/ignore`

## Idle round-robin

- Activates after `kIdleRoundRobinAfterMs = 20s` of no interaction
- Cycles through `g_idle_round[11]` (shuffled), interval 10–22s
- WINK in idle: no scene switch, just triggers `g_wink_start_ms`
- `shuffle_idle_round()` ensures GLITCH is always followed by CONFUSED

## Overlays (LVGL widgets above canvases)

- `g_tongue_obj`: red oval (44×82, radius 22) at (158,222) — visible only for HAPPY_TONGUE
- `g_confused_label`: "??" floating label — visible for CONFUSED/MAIL
- `g_sleep_labels[3]`: "Z/ZZ/ZZZ" — visible during SLEEPY pre-sleep

## PC Agent (pc_agent/agent_gui.py)

- Loads emotion list dynamically from BLE on connect (`LIST_EMOTIONS` characteristic)
- `BASE_EMOTIONS` fallback list exists but is not used after BLE connect
- Handles Windows notifications → MAIL/CALL/TEAMS events
