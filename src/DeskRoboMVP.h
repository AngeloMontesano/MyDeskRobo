#pragma once

#include <Arduino.h>

typedef enum {
  DESKROBO_EMOTION_IDLE = 0,
  DESKROBO_EMOTION_HAPPY,
  DESKROBO_EMOTION_SAD,
  DESKROBO_EMOTION_ANGRY,
  DESKROBO_EMOTION_WOW,
  DESKROBO_EMOTION_SLEEPY,
  DESKROBO_EMOTION_CONFUSED,
  DESKROBO_EMOTION_EXCITED,
  DESKROBO_EMOTION_DIZZY,
  DESKROBO_EMOTION_MAIL,
  DESKROBO_EMOTION_CALL,
  DESKROBO_EMOTION_SHAKE,
  DESKROBO_EMOTION_ANGRY_1,
  DESKROBO_EMOTION_ANGRY_2,
  DESKROBO_EMOTION_ANGRY_3,
  DESKROBO_EMOTION_ANGRY_4,
  DESKROBO_EMOTION_ANGRY_5,
  DESKROBO_EMOTION_ANGRY_6,
  DESKROBO_EMOTION_ANGRY_7,
  DESKROBO_EMOTION_ANGRY_8,
  DESKROBO_EMOTION_ANGRY_9,
  DESKROBO_EMOTION_ANGRY_10,
  DESKROBO_EMOTION_SAD_1,
  DESKROBO_EMOTION_SAD_2,
  DESKROBO_EMOTION_SAD_3,
  DESKROBO_EMOTION_SAD_4,
  DESKROBO_EMOTION_SAD_5,
  DESKROBO_EMOTION_SAD_6,
  DESKROBO_EMOTION_SAD_7,
  DESKROBO_EMOTION_SAD_8,
  DESKROBO_EMOTION_SAD_9,
  DESKROBO_EMOTION_SAD_10,
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
  DESKROBO_STYLE_FLUX = 2,
  DESKROBO_STYLE_ANGRY = 7,
  DESKROBO_STYLE_SAD = 8,
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
void DeskRobo_SetStatusLabelVisible(bool visible);
bool DeskRobo_GetStatusLabelVisible();
void DeskRobo_SetBleConnected(bool connected);


