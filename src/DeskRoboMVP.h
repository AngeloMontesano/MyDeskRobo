#pragma once

#include <Arduino.h>

typedef enum {
  DESKROBO_EMOTION_IDLE = 0,
  DESKROBO_EMOTION_HAPPY,
  DESKROBO_EMOTION_SAD,
  DESKROBO_EMOTION_ANGRY,
  DESKROBO_EMOTION_ANGRY_SOFT,
  DESKROBO_EMOTION_ANGRY_HARD,
  DESKROBO_EMOTION_WOW,
  DESKROBO_EMOTION_SLEEPY,
  DESKROBO_EMOTION_CONFUSED,
  DESKROBO_EMOTION_EXCITED,
  DESKROBO_EMOTION_DIZZY,
  DESKROBO_EMOTION_MAIL,
  DESKROBO_EMOTION_CALL,
  DESKROBO_EMOTION_SHAKE,
  DESKROBO_EMOTION_WINK,
  DESKROBO_EMOTION_XX,
  DESKROBO_EMOTION_GLITCH,
  DESKROBO_EMOTION_BORED,
  DESKROBO_EMOTION_FOCUSED,
  DESKROBO_EMOTION_HAPPY_TONGUE,
} DeskRoboEmotion;

typedef enum {
  DESKROBO_EVENT_NONE = 0,
  DESKROBO_EVENT_AUDIO_QUIET,
  DESKROBO_EVENT_AUDIO_LOUD,
  DESKROBO_EVENT_AUDIO_VERY_LOUD,
  DESKROBO_EVENT_MOTION_TILT,
  DESKROBO_EVENT_MOTION_SHAKE,
  DESKROBO_EVENT_MOTION_TAP,
  DESKROBO_EVENT_PC_CALL,
  DESKROBO_EVENT_PC_MAIL,
  DESKROBO_EVENT_PC_TEAMS,
} DeskRoboEventType;

typedef enum {
  DESKROBO_STYLE_EVE = 1,
} DeskRoboFaceStyle;

void DeskRobo_Init();
void DeskRobo_Loop();
void DeskRobo_PushEvent(DeskRoboEventType event_type);
DeskRoboEmotion DeskRobo_GetEmotion();
void DeskRobo_SetEmotion(DeskRoboEmotion emotion, uint32_t hold_ms);
void DeskRobo_SetEyePair(DeskRoboEmotion left, DeskRoboEmotion right, uint32_t hold_ms);
bool DeskRobo_SetTuning(const char *key, int value);
String DeskRobo_GetTuningJson();
bool DeskRobo_SaveTuning();
bool DeskRobo_LoadTuning();
bool DeskRobo_SetStyleByName(const char *name);
const char *DeskRobo_GetStyleName();
void DeskRobo_SetBacklightLevel(uint8_t value);
void DeskRobo_SetStatusLabelVisible(bool visible);
bool DeskRobo_GetStatusLabelVisible();
void DeskRobo_SetBleConnected(bool connected);
