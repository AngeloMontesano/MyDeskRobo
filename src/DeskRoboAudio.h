#pragma once

#include <Arduino.h>

void DeskRoboAudio_Init();
void DeskRoboAudio_Loop();

bool DeskRoboAudio_MicReady();
bool DeskRoboAudio_SpeakerReady();
float DeskRoboAudio_MicRms();
float DeskRoboAudio_MicPeak();

bool DeskRoboAudio_Beep(uint16_t hz, uint16_t duration_ms);
void DeskRoboAudio_SetBeepLevel(uint16_t level);
