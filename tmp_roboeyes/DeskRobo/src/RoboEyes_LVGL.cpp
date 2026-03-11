/*
 * RoboEyes_LVGL.cpp
 * Port of FluxGarage RoboEyes → LVGL 8.3 / ESP32-S3 / ST77916 360×360
 */

#include "RoboEyes_LVGL.h"
#include <stdlib.h>   // rand()
#include <string.h>   // memset

// ── Canvas buffer size: 360*360*2 bytes (RGB565) ─────────────────────────────
#define CANVAS_BUF_SIZE (LV_CANVAS_BUF_SIZE_TRUE_COLOR(360, 360))

// ─────────────────────────────────────────────────────────────────────────────
//  begin()
// ─────────────────────────────────────────────────────────────────────────────
void RoboEyes_LVGL::begin(lv_obj_t* parent, int width, int height, uint8_t frameRate) {
    _screenW = width;
    _screenH = height;
    _frameInterval = 1000 / frameRate;

    // Allocate canvas buffer in PSRAM
    _cbuf = (lv_color_t*) heap_caps_malloc(
        LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    if (!_cbuf) {
        // Fallback to internal RAM (might be tight)
        _cbuf = (lv_color_t*) malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height));
    }

    _canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(_canvas, _cbuf,
                         width, height, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(_canvas, LV_ALIGN_CENTER, 0, 0);

    // Clear to background color
    lv_canvas_fill_bg(_canvas, _bgColor, LV_OPA_COVER);

    // Scale eye defaults to this display
    // Original defaults: eyeW=36, eyeH=36, space=10, screen=128×64
    // Scale factor relative to 360×360: use shorter side ratio
    float scaleX = (float)width  / 128.0f;
    float scaleY = (float)height /  64.0f;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;
    // Use a slightly smaller scale so eyes fit nicely on round display
    scale *= 0.75f;

    eyeLwidthDefault  = (int)(36 * scale);
    eyeLheightDefault = (int)(36 * scale);
    eyeRwidthDefault  = eyeLwidthDefault;
    eyeRheightDefault = eyeLheightDefault;
    eyeLborderRadiusDefault = (int)(8 * scale);
    eyeRborderRadiusDefault = eyeLborderRadiusDefault;
    spaceBetweenDefault = (int)(10 * scale);

    eyeLwidthCurrent = eyeLwidthDefault;
    eyeRwidthCurrent = eyeRwidthDefault;
    eyeLheightCurrent = 1;  // start closed like original
    eyeRheightCurrent = 1;
    eyeLwidthNext  = eyeLwidthDefault;
    eyeLheightNext = eyeLheightDefault;
    eyeRwidthNext  = eyeRwidthDefault;
    eyeRheightNext = eyeRheightDefault;
    eyeLborderRadiusCurrent = eyeLborderRadiusDefault;
    eyeRborderRadiusCurrent = eyeRborderRadiusDefault;
    eyeLborderRadiusNext    = eyeLborderRadiusDefault;
    eyeRborderRadiusNext    = eyeRborderRadiusDefault;
    spaceBetweenCurrent = spaceBetweenDefault;
    spaceBetweenNext    = spaceBetweenDefault;

    _recalcDefaultPositions();
    eyeLx = eyeLxNext;  eyeLy = eyeLyNext;
    eyeRx = eyeRxNext;  eyeRy = eyeRyNext;

    // Sweat initial positions
    sweat2Xi = width / 2 - 10;
    sweat3Xi = width - 30;

    open(); // queue eyes to open on first draw
}

void RoboEyes_LVGL::_recalcDefaultPositions() {
    eyeLxNext = (_screenW - (eyeLwidthDefault + spaceBetweenDefault + eyeRwidthDefault)) / 2;
    eyeLyNext = (_screenH - eyeLheightDefault) / 2;
    eyeRxNext = eyeLxNext + eyeLwidthCurrent + spaceBetweenCurrent;
    eyeRyNext = eyeLyNext;
}

// ─────────────────────────────────────────────────────────────────────────────
//  update() — rate-limited
// ─────────────────────────────────────────────────────────────────────────────
void RoboEyes_LVGL::update() {
    uint32_t now = lv_tick_get();
    if (now - _fpsTimer >= (uint32_t)_frameInterval) {
        drawEyes();
        _fpsTimer = now;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  setters
// ─────────────────────────────────────────────────────────────────────────────
void RoboEyes_LVGL::setDisplayColors(lv_color_t background, lv_color_t main) {
    _bgColor  = background;
    _eyeColor = main;
}

void RoboEyes_LVGL::setWidth(int l, int r) {
    eyeLwidthNext = l; eyeLwidthDefault = l;
    eyeRwidthNext = r; eyeRwidthDefault = r;
}
void RoboEyes_LVGL::setHeight(int l, int r) {
    eyeLheightNext = l; eyeLheightDefault = l;
    eyeRheightNext = r; eyeRheightDefault = r;
}
void RoboEyes_LVGL::setBorderradius(int l, int r) {
    eyeLborderRadiusNext = l; eyeLborderRadiusDefault = l;
    eyeRborderRadiusNext = r; eyeRborderRadiusDefault = r;
}
void RoboEyes_LVGL::setSpacebetween(int s) {
    spaceBetweenNext = s; spaceBetweenDefault = s;
}
void RoboEyes_LVGL::setCuriosity(bool on) { curious = on; }
void RoboEyes_LVGL::setCyclops(bool on)   { cyclops = on; }
void RoboEyes_LVGL::setSweat(bool on)     { sweat   = on; }

void RoboEyes_LVGL::setMood(uint8_t mood) {
    tired = (mood == RE_TIRED);
    angry = (mood == RE_ANGRY);
    happy = (mood == RE_HAPPY);
}

void RoboEyes_LVGL::setPosition(uint8_t pos) {
    int cx = getScreenConstraint_X() / 2;
    int cy = getScreenConstraint_Y() / 2;
    switch (pos) {
        case RE_N:  eyeLxNext=cx;                    eyeLyNext=0;  break;
        case RE_NE: eyeLxNext=getScreenConstraint_X();eyeLyNext=0; break;
        case RE_E:  eyeLxNext=getScreenConstraint_X();eyeLyNext=cy;break;
        case RE_SE: eyeLxNext=getScreenConstraint_X();eyeLyNext=getScreenConstraint_Y();break;
        case RE_S:  eyeLxNext=cx; eyeLyNext=getScreenConstraint_Y();break;
        case RE_SW: eyeLxNext=0;  eyeLyNext=getScreenConstraint_Y();break;
        case RE_W:  eyeLxNext=0;  eyeLyNext=cy;break;
        case RE_NW: eyeLxNext=0;  eyeLyNext=0; break;
        default:    eyeLxNext=cx; eyeLyNext=cy;break;
    }
}

void RoboEyes_LVGL::open()  { eyeL_open=1; eyeR_open=1; }
void RoboEyes_LVGL::close() { eyeLheightNext=1; eyeRheightNext=1; eyeL_open=0; eyeR_open=0; }
void RoboEyes_LVGL::blink() { close(); open(); }

void RoboEyes_LVGL::open(bool l, bool r) {
    if(l) eyeL_open=1;
    if(r) eyeR_open=1;
}
void RoboEyes_LVGL::close(bool l, bool r) {
    if(l){eyeLheightNext=1; eyeL_open=0;}
    if(r){eyeRheightNext=1; eyeR_open=0;}
}
void RoboEyes_LVGL::blink(bool l, bool r) { close(l,r); open(l,r); }

void RoboEyes_LVGL::setAutoblinker(bool active, int interval, int variation) {
    autoblinker=active; blinkInterval=interval; blinkIntervalVariation=variation;
}
void RoboEyes_LVGL::setAutoblinker(bool active) { autoblinker=active; }

void RoboEyes_LVGL::setIdleMode(bool active, int interval, int variation) {
    idle=active; idleInterval=interval; idleIntervalVariation=variation;
}
void RoboEyes_LVGL::setIdleMode(bool active) { idle=active; }

void RoboEyes_LVGL::setHFlicker(bool on, uint8_t amp) { hFlicker=on; hFlickerAmplitude=amp; }
void RoboEyes_LVGL::setHFlicker(bool on)              { hFlicker=on; }
void RoboEyes_LVGL::setVFlicker(bool on, uint8_t amp) { vFlicker=on; vFlickerAmplitude=amp; }
void RoboEyes_LVGL::setVFlicker(bool on)              { vFlicker=on; }

void RoboEyes_LVGL::anim_confused() { confused=1; }
void RoboEyes_LVGL::anim_laugh()    { laugh=1; }

int RoboEyes_LVGL::getScreenConstraint_X() {
    return _screenW - eyeLwidthCurrent - spaceBetweenCurrent - eyeRwidthCurrent;
}
int RoboEyes_LVGL::getScreenConstraint_Y() {
    return _screenH - eyeLheightDefault;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Internal drawing helpers (lv_canvas pixel operations)
// ─────────────────────────────────────────────────────────────────────────────

void RoboEyes_LVGL::_fillRoundRect(int x, int y, int w, int h, int r, lv_color_t col) {
    if (w<=0 || h<=0) return;
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color   = col;
    dsc.bg_opa     = LV_OPA_COVER;
    dsc.radius     = r;
    dsc.border_width = 0;
    lv_area_t area = {(lv_coord_t)x, (lv_coord_t)y,
                      (lv_coord_t)(x+w-1), (lv_coord_t)(y+h-1)};
    lv_canvas_draw_rect(_canvas, x, y, w, h, &dsc);
}

void RoboEyes_LVGL::_fillTriangle(int x0,int y0, int x1,int y1, int x2,int y2, lv_color_t col) {
    // Rasterize triangle using scanline fill
    // Sort by y
    if (y0 > y1) { int t; t=x0;x0=x1;x1=t; t=y0;y0=y1;y1=t; }
    if (y0 > y2) { int t; t=x0;x0=x2;x2=t; t=y0;y0=y2;y2=t; }
    if (y1 > y2) { int t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; }

    lv_draw_line_dsc_t ldsc;
    lv_draw_line_dsc_init(&ldsc);
    ldsc.color = col;
    ldsc.width = 1;
    ldsc.opa   = LV_OPA_COVER;

    int totalH = y2 - y0;
    if (totalH == 0) return;

    for (int i = 0; i <= totalH; i++) {
        bool second = (i > y1 - y0) || (y1 == y0);
        int segH = second ? (y2 - y1) : (y1 - y0);
        if (segH == 0) segH = 1;

        int ax = x0 + (int)((float)(x2 - x0) * i / totalH);
        int bx;
        if (second) {
            bx = x1 + (int)((float)(x2 - x1) * (i - (y1 - y0)) / segH);
        } else {
            bx = x0 + (int)((float)(x1 - x0) * i / segH);
        }
        int scanY = y0 + i;
        if (ax > bx) { int t=ax; ax=bx; bx=t; }
        // Draw horizontal line segment
        lv_canvas_draw_line(_canvas, (lv_point_t[]){ {(lv_coord_t)ax,(lv_coord_t)scanY},
                                                      {(lv_coord_t)bx,(lv_coord_t)scanY} }, 2, &ldsc);
    }
}

void RoboEyes_LVGL::_fillEllipse(int cx, int cy, int rx, int ry, lv_color_t col) {
    if (rx<=0 || ry<=0) return;
    _fillRoundRect(cx-rx, cy-ry, rx*2, ry*2, LV_RADIUS_CIRCLE, col);
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawEyes() — main drawing routine, ported 1:1 from original
// ─────────────────────────────────────────────────────────────────────────────
void RoboEyes_LVGL::drawEyes() {
    if (!_canvas) return;

    uint32_t now = lv_tick_get();

    //// PRE-CALCULATIONS — identical logic to original ////

    // Curious mode height offset
    if (curious) {
        eyeLheightOffset = (eyeLxNext <= 10) ? 22 : 0;
        if (eyeRxNext >= _screenW - eyeRwidthCurrent - 10) eyeRheightOffset = 22;
        else eyeRheightOffset = 0;
    } else {
        eyeLheightOffset = 0;
        eyeRheightOffset = 0;
    }

    // Height tweening (exponential ease: current = (current+next)/2)
    eyeLheightCurrent = (eyeLheightCurrent + eyeLheightNext + eyeLheightOffset) / 2;
    eyeLy += (eyeLheightDefault - eyeLheightCurrent) / 2;
    eyeLy -= eyeLheightOffset / 2;

    eyeRheightCurrent = (eyeRheightCurrent + eyeRheightNext + eyeRheightOffset) / 2;
    eyeRy += (eyeRheightDefault - eyeRheightCurrent) / 2;
    eyeRy -= eyeRheightOffset / 2;

    // Reopen after close
    if (eyeL_open && eyeLheightCurrent <= 1 + eyeLheightOffset) eyeLheightNext = eyeLheightDefault;
    if (eyeR_open && eyeRheightCurrent <= 1 + eyeRheightOffset) eyeRheightNext = eyeRheightDefault;

    // Width tweening
    eyeLwidthCurrent = (eyeLwidthCurrent + eyeLwidthNext) / 2;
    eyeRwidthCurrent = (eyeRwidthCurrent + eyeRwidthNext) / 2;

    // Space tweening
    spaceBetweenCurrent = (spaceBetweenCurrent + spaceBetweenNext) / 2;

    // Position tweening
    eyeLx = (eyeLx + eyeLxNext) / 2;
    eyeLy = (eyeLy + eyeLyNext) / 2;
    eyeRxNext = eyeLxNext + eyeLwidthCurrent + spaceBetweenCurrent;
    eyeRyNext = eyeLyNext;
    eyeRx = (eyeRx + eyeRxNext) / 2;
    eyeRy = (eyeRy + eyeRyNext) / 2;

    // Border radius tweening
    eyeLborderRadiusCurrent = (eyeLborderRadiusCurrent + eyeLborderRadiusNext) / 2;
    eyeRborderRadiusCurrent = (eyeRborderRadiusCurrent + eyeRborderRadiusNext) / 2;

    // Auto-blink
    if (autoblinker && now >= blinktimer) {
        blink();
        blinktimer = now + (uint32_t)(blinkInterval * 1000)
                         + (uint32_t)((rand() % blinkIntervalVariation) * 1000);
    }

    // Laugh animation
    if (laugh) {
        if (laughToggle) { setVFlicker(1,5); laughAnimationTimer=now; laughToggle=0; }
        else if (now >= laughAnimationTimer + (uint32_t)laughAnimationDuration) {
            setVFlicker(0,0); laughToggle=1; laugh=0;
        }
    }

    // Confused animation
    if (confused) {
        if (confusedToggle) { setHFlicker(1,20); confusedAnimationTimer=now; confusedToggle=0; }
        else if (now >= confusedAnimationTimer + (uint32_t)confusedAnimationDuration) {
            setHFlicker(0,0); confusedToggle=1; confused=0;
        }
    }

    // Idle
    if (idle && now >= idleAnimationTimer) {
        eyeLxNext = rand() % (getScreenConstraint_X() + 1);
        eyeLyNext = rand() % (getScreenConstraint_Y() + 1);
        idleAnimationTimer = now + (uint32_t)(idleInterval * 1000)
                                 + (uint32_t)((rand() % idleIntervalVariation) * 1000);
    }

    // Flicker offsets
    if (hFlicker) {
        int offset = hFlickerAlternate ? hFlickerAmplitude : -hFlickerAmplitude;
        eyeLx += offset; eyeRx += offset;
        hFlickerAlternate = !hFlickerAlternate;
    }
    if (vFlicker) {
        int offset = vFlickerAlternate ? vFlickerAmplitude : -vFlickerAmplitude;
        eyeLy += offset; eyeRy += offset;
        vFlickerAlternate = !vFlickerAlternate;
    }

    // Cyclops
    if (cyclops) { eyeRwidthCurrent=0; eyeRheightCurrent=0; spaceBetweenCurrent=0; }


    //// ACTUAL DRAWINGS ////

    // 1) Clear canvas
    lv_canvas_fill_bg(_canvas, _bgColor, LV_OPA_COVER);

    // 2) Draw eyes (filled rounded rectangles)
    _fillRoundRect(eyeLx, eyeLy, eyeLwidthCurrent, eyeLheightCurrent,
                   eyeLborderRadiusCurrent, _eyeColor);
    if (!cyclops) {
        _fillRoundRect(eyeRx, eyeRy, eyeRwidthCurrent, eyeRheightCurrent,
                       eyeRborderRadiusCurrent, _eyeColor);
    }

    // 3) Inner eye highlight (bonus for color display — not in original)
    //    Small ellipse in upper part of each eye
    {
        int hlRx = eyeLwidthCurrent  / 4;
        int hlRy = eyeLheightCurrent / 5;
        if (hlRx > 1 && hlRy > 1) {
            int hlCx = eyeLx + eyeLwidthCurrent / 2 - eyeLwidthCurrent / 8;
            int hlCy = eyeLy + eyeLheightCurrent / 3;
            _fillEllipse(hlCx, hlCy, hlRx, hlRy, lv_color_hex(0xCCEEFF));
        }
        if (!cyclops) {
            int hlRx2 = eyeRwidthCurrent  / 4;
            int hlRy2 = eyeRheightCurrent / 5;
            if (hlRx2 > 1 && hlRy2 > 1) {
                int hlCx2 = eyeRx + eyeRwidthCurrent / 2 - eyeRwidthCurrent / 8;
                int hlCy2 = eyeRy + eyeRheightCurrent / 3;
                _fillEllipse(hlCx2, hlCy2, hlRx2, hlRy2, lv_color_hex(0xCCEEFF));
            }
        }
    }

    // 4) Eyelid mood transitions
    if (tired)  { eyelidsTiredHeightNext  = eyeLheightCurrent/2; eyelidsAngryHeightNext = 0; }
    else        { eyelidsTiredHeightNext  = 0; }
    if (angry)  { eyelidsAngryHeightNext  = eyeLheightCurrent/2; eyelidsTiredHeightNext = 0; }
    else        { eyelidsAngryHeightNext  = 0; }
    if (happy)  { eyelidsHappyBottomOffsetNext = eyeLheightCurrent/2; }
    else        { eyelidsHappyBottomOffsetNext = 0; }

    // 5) Draw tired top eyelids (triangle, inner corners down)
    eyelidsTiredHeight = (eyelidsTiredHeight + eyelidsTiredHeightNext) / 2;
    if (eyelidsTiredHeight > 0.5f) {
        int h = (int)eyelidsTiredHeight;
        if (!cyclops) {
            // Left eye: left corner goes down
            _fillTriangle(eyeLx, eyeLy-1,
                          eyeLx+eyeLwidthCurrent, eyeLy-1,
                          eyeLx, eyeLy+h-1, _bgColor);
            // Right eye: right corner goes down
            _fillTriangle(eyeRx, eyeRy-1,
                          eyeRx+eyeRwidthCurrent, eyeRy-1,
                          eyeRx+eyeRwidthCurrent, eyeRy+h-1, _bgColor);
        } else {
            _fillTriangle(eyeLx, eyeLy-1,
                          eyeLx+eyeLwidthCurrent/2, eyeLy-1,
                          eyeLx, eyeLy+h-1, _bgColor);
            _fillTriangle(eyeLx+eyeLwidthCurrent/2, eyeLy-1,
                          eyeLx+eyeLwidthCurrent, eyeLy-1,
                          eyeLx+eyeLwidthCurrent, eyeLy+h-1, _bgColor);
        }
    }

    // 6) Draw angry top eyelids (triangle, outer corners down)
    eyelidsAngryHeight = (eyelidsAngryHeight + eyelidsAngryHeightNext) / 2;
    if (eyelidsAngryHeight > 0.5f) {
        int h = (int)eyelidsAngryHeight;
        if (!cyclops) {
            // Left eye: right corner goes down
            _fillTriangle(eyeLx, eyeLy-1,
                          eyeLx+eyeLwidthCurrent, eyeLy-1,
                          eyeLx+eyeLwidthCurrent, eyeLy+h-1, _bgColor);
            // Right eye: left corner goes down
            _fillTriangle(eyeRx, eyeRy-1,
                          eyeRx+eyeRwidthCurrent, eyeRy-1,
                          eyeRx, eyeRy+h-1, _bgColor);
        } else {
            _fillTriangle(eyeLx, eyeLy-1,
                          eyeLx+eyeLwidthCurrent/2, eyeLy-1,
                          eyeLx+eyeLwidthCurrent/2, eyeLy+h-1, _bgColor);
            _fillTriangle(eyeLx+eyeLwidthCurrent/2, eyeLy-1,
                          eyeLx+eyeLwidthCurrent, eyeLy-1,
                          eyeLx+eyeLwidthCurrent/2, eyeLy+h-1, _bgColor);
        }
    }

    // 7) Happy bottom eyelids (rounded rect overlay from bottom)
    eyelidsHappyBottomOffset = (eyelidsHappyBottomOffset + eyelidsHappyBottomOffsetNext) / 2;
    if (eyelidsHappyBottomOffset > 0.5f) {
        int off = (int)eyelidsHappyBottomOffset;
        _fillRoundRect(eyeLx-1,
                       (eyeLy+eyeLheightCurrent) - off + 1,
                       eyeLwidthCurrent+2, eyeLheightDefault,
                       eyeLborderRadiusCurrent, _bgColor);
        if (!cyclops) {
            _fillRoundRect(eyeRx-1,
                           (eyeRy+eyeRheightCurrent) - off + 1,
                           eyeRwidthCurrent+2, eyeRheightDefault,
                           eyeRborderRadiusCurrent, _bgColor);
        }
    }

    // 8) Sweat drops (same logic as original, scaled)
    if (sweat) {
        auto& s1y=sweat1YPos; auto& s1m=sweat1YPosMax; auto& s1w=sweat1W; auto& s1h=sweat1H; auto& s1x=sweat1X;
        if(s1y<=s1m){s1y+=1.0f;} else{sweat1Xi=rand()%60; s1y=2; s1m=rand()%20+10; s1w=2; s1h=4; s1x=sweat1Xi;}
        if(s1y<=s1m/2){s1w+=0.5f;s1h+=0.5f;} else{s1w-=0.2f;s1h-=1.0f;}
        s1x=sweat1Xi-(int)(s1w/2);
        _fillRoundRect(s1x,(int)s1y,(int)s1w,(int)s1h,3,_eyeColor);

        auto& s2y=sweat2YPos; auto& s2m=sweat2YPosMax; auto& s2w=sweat2W; auto& s2h=sweat2H; auto& s2x=sweat2X;
        if(s2y<=s2m){s2y+=1.0f;} else{sweat2Xi=rand()%(_screenW-60)+30; s2y=2; s2m=rand()%20+10; s2w=2; s2h=4; s2x=sweat2Xi;}
        if(s2y<=s2m/2){s2w+=0.5f;s2h+=0.5f;} else{s2w-=0.2f;s2h-=1.0f;}
        s2x=sweat2Xi-(int)(s2w/2);
        _fillRoundRect(s2x,(int)s2y,(int)s2w,(int)s2h,3,_eyeColor);

        auto& s3y=sweat3YPos; auto& s3m=sweat3YPosMax; auto& s3w=sweat3W; auto& s3h=sweat3H; auto& s3x=sweat3X;
        if(s3y<=s3m){s3y+=1.0f;} else{sweat3Xi=(_screenW-60)+(rand()%60); s3y=2; s3m=rand()%20+10; s3w=2; s3h=4; s3x=sweat3Xi;}
        if(s3y<=s3m/2){s3w+=0.5f;s3h+=0.5f;} else{s3w-=0.2f;s3h-=1.0f;}
        s3x=sweat3Xi-(int)(s3w/2);
        _fillRoundRect(s3x,(int)s3y,(int)s3w,(int)s3h,3,_eyeColor);
    }

    // Canvas is displayed automatically by LVGL on next lv_task_handler() call
}
