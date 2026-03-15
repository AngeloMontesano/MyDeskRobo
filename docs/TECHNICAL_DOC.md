# MyDeskRobo Technical Documentation

## 1. Overview

Current public runtime:
- firmware target: `esp32-s3-mydeskrobo-full`
- renderer: `MyDeskRoboEngine`
- control path: BLE
- companion app: Windows PC agent

The current release is BLE-only in normal operation.

## 2. Runtime Architecture

Main loop responsibilities:
- display and LVGL bring-up
- MyDeskRobo runtime state machine
- BLE command loop
- local gyro motion loop
- LVGL flush loop

Key modules:
- `src/DeskRoboMVP_nextgen.cpp`
  - runtime state, active emotion, idle rotation, persistence, sleep logic
- `MyDeskRoboEngine/include/scenes/*.h`
  - scene definitions (one file per emotion)
- `MyDeskRoboEngine/src/LayerRenderer.cpp`
  - canvas-based renderer (DrawGlow/Mid/Core/Cut/Brow/Shape primitives)
- `src/DeskRoboBLE.cpp`
  - BLE parser and queued command handoff
- `src/main.cpp`
  - hardware bring-up, gyro/motion loop, BLE startup
- `pc_agent/*`
  - Windows control app and BLE transport

## 3. Face Model

Style: `EVE` only (single canvas per eye, monochromatic)

### Active emotions

| BLE name | Scene file | Notes |
|----------|------------|-------|
| IDLE | eve_idle.h | default |
| HAPPY | eve_happy.h | |
| SAD | eve_sad.h | |
| ANGRY | eve_angry_hard.h | alias for ANGRY_HARD |
| ANGRY_SOFT | eve_angry_soft.h | |
| ANGRY_HARD | eve_angry_hard.h | no pupils, hard scowl brow |
| WOW | eve_wow.h | |
| SLEEPY | eve_sleepy.h | |
| CONFUSED | eve_confused.h | floating "??" overlay |
| DIZZY | eve_dizzy.h | X-eyes via two crossing DrawCut bars at ±42° |
| MAIL | eve_confused.h | alias for CONFUSED |
| CALL | eve_call.h | |
| SHAKE | eve_shake.h | random pupil jitter |
| WINK | eve_wink.h | animated: right eye closes/opens (no fade-in) |
| XX | eve_xx.h | |
| GLITCH | eve_glitch.h | glitch FX overlay |
| BORED | eve_bored.h | eye-roll animation (pupils drift up over 2.5 s) |
| FOCUSED | eve_focused.h | pupils scan left→right together |
| HAPPY_TONGUE | eve_happy_tongue.h | base happy + red tongue LVGL widget |
| HOLLOW | eve_hollow.h | glowing ring iris (DrawCut removes interior) |

Eye-pair override: left and right canvases can show different emotions via BLE.

### WINK animation
- No fade-in when switching to WINK — appears instantly
- Right eye animates: 80 ms close → 70 ms hold → 150 ms open (via `output_opa`)
- Left eye stays fully open throughout
- Auto-expires and fades back to previous emotion

## 4. Event Logic

Events map into temporary emotions with priority and TTL:

| Event | Emotion | Priority | TTL |
|-------|---------|----------|-----|
| PC_CALL | CALL | 90 | 5 s |
| PC_MAIL | CONFUSED | 70 | 4 s |
| PC_TEAMS | WOW | 70 | 4 s |
| AUDIO_LOUD | WOW | 60 | 2.5 s |
| AUDIO_VERY_LOUD | SHAKE | 85 | 2.6 s |
| MOTION_TILT | CONFUSED | 55 | 2.2 s |
| MOTION_SHAKE | SHAKE | 85 | 2.6 s |
| MOTION_TAP | cycles idle round | 60 | 5 s |

### Gyro motion detection (main.cpp)
- `DESKROBO_ENABLE_GYRO` build flag must be 1 (default 0)
- Poll interval: 40 ms
- Burst hit counting: `g_burst_hits` (shake_sample=true count per burst)
- TAP: `hits <= 6`, cooldown > 600 ms → cycles to next idle emotion
- SHAKE: `hits > 6`, burst >= 500 ms, cooldown > 2600 ms
- Serial log: `[GYRO] burst=Xms hits=Y → TAP/SHAKE/ignore`

## 5. BLE Control Path

BLE does not change LVGL directly. BLE callbacks only enqueue commands for the main loop.

Supported command families:
- emotion / eye-pair
- event
- backlight
- status label
- tuning (key/value)
- save/load tuning
- time sync
- factory reset
- LIST_EMOTIONS (returns current emotion list to PC agent on connect)

## 6. Persistence

Preferences namespace: `deskrobo`

Persisted values include:
- eye motion tuning (drift, saccade, glow)
- blink timing
- shake/gyro thresholds
- sleep/display-off timing
- eye color
- face style

## 7. Fade-to-Black Transition

When switching emotions:
- `g_render_emotion` trails `g_current_emotion`
- Phase 1 (0–80 ms): canvas fades out (output_opa 255→1)
- Phase 2 (80–160 ms): new scene fades in (output_opa 1→255)
- `output_opa == 0` is treated as COVER by LVGL — always clamp to min 1

WINK bypasses the fade-in (appears instantly).

## 8. Boot and Power Behavior

Boot sequence:
- hardware power hold (GPIO 7)
- I2C / expander / battery / SD init
- optional gyro init
- backlight init → LCD init → LVGL init
- MyDeskRobo runtime init
- Audio init
- BLE task start (separate FreeRTOS task)

Boot UI:
- splash screen with logo
- `My Robo Desk` / `v0.5`
- splash hidden after ~2.8 s

## 9. Build and Flash

Build:
```powershell
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUTF8='1'
chcp 65001 > $null
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Upload:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

Erase NVS / full flash:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t erase
```

## 10. Stability Notes

- Canvas renderer is required — do not reintroduce rotated LVGL object layering.
- Old saved preferences can override new firmware defaults; erase or factory-reset if behavior looks wrong after flashing.
- Gyro thresholds must stay conservative; false shake events are worse than missing weak taps.
- `output_opa == 0` in LayerRenderer means COVER (LVGL fallback) — always clamp fade values to min 1.
