---
name: project_emotions
description: Current emotion list, scene files, and special behaviors
type: project
---

## Active emotions (as of 2026-03-15)

| Enum | BLE name | Scene file | Notes |
|------|----------|------------|-------|
| IDLE | IDLE | eve_idle.h | default |
| HAPPY | HAPPY | eve_happy.h | |
| SAD | SAD | eve_sad.h | |
| ANGRY | ANGRY | eve_angry_hard.h | alias for ANGRY_HARD |
| ANGRY_SOFT | ANGRY_SOFT | eve_angry_soft.h | |
| ANGRY_HARD | ANGRY_HARD | eve_angry_hard.h | no pupils |
| WOW | WOW | eve_wow.h | |
| SLEEPY | SLEEPY | eve_sleepy.h | |
| CONFUSED | CONFUSED | eve_confused.h | floating "??" overlay |
| DIZZY | DIZZY | eve_dizzy.h | X-eyes (two crossing DrawCut bars at ±42°) |
| MAIL | MAIL | eve_confused.h | alias |
| CALL | CALL | eve_call.h | |
| SHAKE | SHAKE | eve_shake.h | triggered by long shake gesture |
| WINK | WINK | eve_wink.h | animated: right eye closes/opens, no fade-in |
| XX | XX | eve_xx.h | |
| GLITCH | GLITCH | eve_glitch.h | |
| BORED | BORED | eve_bored.h | eye-roll animation |
| FOCUSED | FOCUSED | eve_focused.h | pupils scan left→right together (mirror_x_for_right=false) |
| HAPPY_TONGUE | HAPPY_TONGUE | eve_happy_tongue.h | red tongue LVGL widget below eyes |
| HOLLOW | HOLLOW | eve_hollow.h | (internal: eve_sunglasses_round) glowing ring iris |

## Removed emotions
- SKEPTICAL — deleted, didn't look convincing
- EXCITED — removed from system
- SUNGLASSES_AVIATOR — deleted

## WINK behavior
- No fade-in: scene appears instantly, `g_render_emotion` set immediately
- `g_wink_start_ms` starts at set time
- Animation: 80ms close → 70ms hold → 150ms open (right eye only via output_opa)
- Left eye stays fully open (base eye shape)
- Auto-expires after kWinkTotalMs + kFadeHalfMs + 40ms

## BLE LIST_EMOTIONS order
`IDLE,HAPPY,SAD,ANGRY,ANGRY_SOFT,ANGRY_HARD,WOW,SLEEPY,CONFUSED,DIZZY,MAIL,CALL,SHAKE,WINK,XX,GLITCH,BORED,FOCUSED,HAPPY_TONGUE,HOLLOW`
