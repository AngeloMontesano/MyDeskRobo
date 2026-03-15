---
name: project_overview
description: What MyDeskRobo is, its stack, and current release state
type: project
---

MyDeskRobo is an animated desk companion on a round ESP32-S3 display (360×360, Waveshare ESP32-S3-LCD-1.85, no touch). It displays expressive animated eyes and reacts to audio, motion (IMU), and BLE commands from a PC agent.

**Stack:**
- PlatformIO + Arduino framework
- LVGL 8.3.10 alpha4 — canvas-based rendering (LayerRenderer), single color per canvas
- QMI8658 IMU (gyro + accelerometer) — optional, `DESKROBO_ENABLE_GYRO` flag
- BLE for PC↔device communication
- SD card + Flash storage

**Key source files:**
- `src/main.cpp` — setup/loop, gyro motion detection
- `src/DeskRoboMVP_nextgen.cpp` — emotion engine, render loop, BLE event handling
- `src/DeskRoboMVP.h` — emotion/event enums, public API
- `src/DeskRoboBLE.cpp` — BLE characteristic handlers, LIST_EMOTIONS, SET_EMOTION
- `src/DeskRoboAudio.cpp` — microphone / audio level events
- `MyDeskRoboEngine/include/scenes/eve_*.h` — per-emotion scene definitions (RenderOp arrays)
- `MyDeskRoboEngine/src/LayerRenderer.cpp` — LVGL canvas renderer
- `pc_agent/agent_gui.py` — Windows PC agent (BLE, system notifications, GUI)

**Current version:** v0.5 (boot splash). Project approaching 1.0.

**Why:** Hobby project, no deadline. Focus is on polish and fun interactions.
**How to apply:** Suggestions should favor visual quality and simplicity over architecture.
