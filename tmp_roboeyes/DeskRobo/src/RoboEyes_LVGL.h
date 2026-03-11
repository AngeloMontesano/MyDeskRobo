/*
 * RoboEyes_LVGL.h
 * 
 * Port of FluxGarage RoboEyes (by Dennis Hoelscher, www.fluxgarage.com)
 * from Adafruit GFX / OLED → LVGL 8.3 color display (ST77916 360x360)
 * 
 * Original: https://github.com/FluxGarage/RoboEyes
 * Port for DeskRobo / ESP32-S3 by Angelo / Claude
 * 
 * API intentionally kept identical to the original where possible.
 * Scale: original 128×64 → 360×360 (factor ~2.8 both axes)
 * 
 * Drawing approach:
 *   - Eyes      : lv_canvas filled rounded rect
 *   - Eyelids   : lv_canvas filled triangles (overlay, drawn in BGCOLOR)
 *   - Highlights: lv_canvas filled ellipse (inner eye shine, color display bonus)
 *   - All drawn into a single lv_canvas each frame (like clearDisplay+draw+display)
 */

#pragma once
#include "lvgl.h"

// ── Mood constants (same as original) ────────────────────────────────────────
#define RE_DEFAULT  0
#define RE_TIRED    1
#define RE_ANGRY    2
#define RE_HAPPY    3

// ── On/Off (same as original) ─────────────────────────────────────────────────
#ifndef ON
#define ON  1
#endif
#ifndef OFF
#define OFF 0
#endif

// ── Position constants (same as original) ────────────────────────────────────
#define RE_N   1
#define RE_NE  2
#define RE_E   3
#define RE_SE  4
#define RE_S   5
#define RE_SW  6
#define RE_W   7
#define RE_NW  8

// ── Color helpers ─────────────────────────────────────────────────────────────
// Default palette — call setDisplayColors() to override
#define RE_COLOR_BG     lv_color_hex(0x04060C)   // near-black background
#define RE_COLOR_EYE    lv_color_hex(0xFFFFFF)   // white eye fill (classic)
#define RE_COLOR_HL     lv_color_hex(0xCCEEFF)   // inner highlight
#define RE_COLOR_HL2    lv_color_hex(0xFFFFFF)   // specular dot


class RoboEyes_LVGL {
public:

    // ── Constructor ──────────────────────────────────────────────────────────
    RoboEyes_LVGL() {}

    // ── Init — call once after lv_init() ────────────────────────────────────
    // parent: lv_scr_act() or any container
    // width/height: display resolution (360,360)
    // frameRate: max FPS for update()
    void begin(lv_obj_t* parent, int width, int height, uint8_t frameRate);

    // ── Main loop call ───────────────────────────────────────────────────────
    void update();        // respects frameRate limit
    void drawEyes();      // draw immediately, no rate limit

    // ── Display colors ───────────────────────────────────────────────────────
    void setDisplayColors(lv_color_t background, lv_color_t main);

    // ── Eye geometry ─────────────────────────────────────────────────────────
    void setWidth(int leftEye, int rightEye);
    void setHeight(int leftEye, int rightEye);
    void setBorderradius(int leftEye, int rightEye);
    void setSpacebetween(int space);

    // ── Mood ─────────────────────────────────────────────────────────────────
    void setMood(uint8_t mood);

    // ── Position ─────────────────────────────────────────────────────────────
    void setPosition(uint8_t position);

    // ── Flags ────────────────────────────────────────────────────────────────
    void setCuriosity(bool on);
    void setCyclops(bool on);

    // ── Blink ────────────────────────────────────────────────────────────────
    void open();
    void close();
    void blink();
    void open(bool left, bool right);
    void close(bool left, bool right);
    void blink(bool left, bool right);

    // ── Auto-blink ───────────────────────────────────────────────────────────
    void setAutoblinker(bool active, int interval, int variation);
    void setAutoblinker(bool active);

    // ── Idle ─────────────────────────────────────────────────────────────────
    void setIdleMode(bool active, int interval, int variation);
    void setIdleMode(bool active);

    // ── Flicker ──────────────────────────────────────────────────────────────
    void setHFlicker(bool on, uint8_t amplitude);
    void setHFlicker(bool on);
    void setVFlicker(bool on, uint8_t amplitude);
    void setVFlicker(bool on);

    // ── Sweat ────────────────────────────────────────────────────────────────
    void setSweat(bool on);

    // ── One-shot animations ──────────────────────────────────────────────────
    void anim_confused();
    void anim_laugh();

    // ── Helpers ──────────────────────────────────────────────────────────────
    int getScreenConstraint_X();
    int getScreenConstraint_Y();

private:

    // Canvas
    lv_obj_t*  _canvas  = nullptr;
    lv_color_t* _cbuf   = nullptr;

    int _screenW = 360;
    int _screenH = 360;
    int _frameInterval = 20;
    uint32_t _fpsTimer = 0;

    // Colors
    lv_color_t _bgColor  = RE_COLOR_BG;
    lv_color_t _eyeColor = RE_COLOR_EYE;

    // ── Eye geometry (mirroring original variable names) ──────────────────
    int eyeLwidthDefault  = 100;
    int eyeLheightDefault = 100;
    int eyeLwidthCurrent  = 100;
    int eyeLheightCurrent = 1;
    int eyeLwidthNext     = 100;
    int eyeLheightNext    = 100;
    int eyeLheightOffset  = 0;
    int eyeLborderRadiusDefault = 22;
    int eyeLborderRadiusCurrent = 22;
    int eyeLborderRadiusNext    = 22;

    int eyeRwidthDefault  = 100;
    int eyeRheightDefault = 100;
    int eyeRwidthCurrent  = 100;
    int eyeRheightCurrent = 1;
    int eyeRwidthNext     = 100;
    int eyeRheightNext    = 100;
    int eyeRheightOffset  = 0;
    int eyeRborderRadiusDefault = 22;
    int eyeRborderRadiusCurrent = 22;
    int eyeRborderRadiusNext    = 22;

    int spaceBetweenDefault = 28;
    int spaceBetweenCurrent = 28;
    int spaceBetweenNext    = 28;

    // Derived positions (recalculated each frame)
    int eyeLx, eyeLy, eyeLxNext, eyeLyNext;
    int eyeRx, eyeRy, eyeRxNext, eyeRyNext;

    // Mood flags
    bool tired   = false;
    bool angry   = false;
    bool happy   = false;
    bool curious = false;
    bool cyclops = false;
    bool eyeL_open = false;
    bool eyeR_open = false;

    // Eyelid tweening
    float eyelidsTiredHeight          = 0;
    float eyelidsTiredHeightNext      = 0;
    float eyelidsAngryHeight          = 0;
    float eyelidsAngryHeightNext      = 0;
    float eyelidsHappyBottomOffset    = 0;
    float eyelidsHappyBottomOffsetNext= 0;

    // Flicker
    bool  hFlicker = false, hFlickerAlternate = false;
    uint8_t hFlickerAmplitude = 4;
    bool  vFlicker = false, vFlickerAlternate = false;
    uint8_t vFlickerAmplitude = 8;

    // Auto-blink
    bool autoblinker = false;
    int  blinkInterval = 2, blinkIntervalVariation = 4;
    uint32_t blinktimer = 0;

    // Idle
    bool idle = false;
    int  idleInterval = 1, idleIntervalVariation = 3;
    uint32_t idleAnimationTimer = 0;

    // Confused
    bool confused = false;
    uint32_t confusedAnimationTimer = 0;
    int  confusedAnimationDuration  = 500;
    bool confusedToggle = true;

    // Laugh
    bool laugh = false;
    uint32_t laughAnimationTimer = 0;
    int  laughAnimationDuration  = 500;
    bool laughToggle = true;

    // Sweat
    bool  sweat = false;
    float sweat1YPos=2, sweat2YPos=2, sweat3YPos=2;
    int   sweat1YPosMax=10, sweat2YPosMax=10, sweat3YPosMax=10;
    float sweat1W=1,sweat1H=2, sweat2W=1,sweat2H=2, sweat3W=1,sweat3H=2;
    int   sweat1X=2,  sweat2X=60, sweat3X=120;
    int   sweat1Xi=2, sweat2Xi=60,sweat3Xi=120;

    // Internal draw helpers
    void _fillRoundRect(int x, int y, int w, int h, int r, lv_color_t col);
    void _fillTriangle(int x0,int y0, int x1,int y1, int x2,int y2, lv_color_t col);
    void _fillEllipse(int cx, int cy, int rx, int ry, lv_color_t col);
    void _recalcDefaultPositions();
};
