#pragma once
#include "lvgl.h"

// ─── Farben ───────────────────────────────────────────────────────────────────
#define COLOR_BACKGROUND   lv_color_make(0x05, 0x07, 0x09)
#define COLOR_HEAD_BG      lv_color_make(0x10, 0x14, 0x18)
#define COLOR_HEAD_BORDER  lv_color_make(0x1A, 0x1F, 0x24)
#define COLOR_VISOR_BG     lv_color_make(0x07, 0x09, 0x0C)
#define COLOR_EYE_GLOW     lv_color_make(0x2D, 0xDC, 0xF0)
#define COLOR_EYE_CORE     lv_color_make(0xCC, 0xF8, 0xFF)
#define COLOR_WHITE        lv_color_white()
#define COLOR_BLACK        lv_color_black()

// ─── Kopf-Dimensionen ─────────────────────────────────────────────────────────
#define HEAD_W        140
#define HEAD_H        100
#define HEAD_RADIUS    25
#define VISOR_PAD       8
#define VISOR_RADIUS   18

// ─── Auge-Canvas-Größe ────────────────────────────────────────────────────────
#define EYE_CANVAS_W   36
#define EYE_CANVAS_H   52
#define EYE_SPACING    16   // Abstand zwischen beiden Augen

// ─── Emotionen ────────────────────────────────────────────────────────────────
typedef enum {
    EYE_IDLE      = 0,   // Standard-Oval
    EYE_HAPPY     = 1,   // Unterer Halbmond (lächeln)
    EYE_SAD       = 2,   // Oval schräg nach innen gekippt
    EYE_ANGRY     = 3,   // Oval mit diagonal abgeschnittener innerer Ecke
    EYE_SURPRISED = 4,   // Größeres rundes Oval
    EYE_SLEEPY    = 5,   // Sehr flacher Schlitz
    EYE_WINK      = 6,   // Links = Bogen, Rechts = Oval
    EYE_LOVE      = 7,   // Herz-Form per Canvas
    EYE_COUNT
} EyeEmotion;

// ─── API ──────────────────────────────────────────────────────────────────────

// Initialisiert alle globalen Styles (einmalig aufrufen)
void robot_eyes_init_styles(void);

// Erstellt das 4×2 Raster aller Emotionen auf dem übergebenen Parent
void robot_eyes_grid_create(lv_obj_t *parent);

// Erstellt einen einzelnen großen Roboterkopf und gibt ihn zurück
// Über robot_eyes_set_emotion() kann die Emotion danach geändert werden
lv_obj_t* robot_eyes_create_face(lv_obj_t *parent);
void      robot_eyes_set_emotion(EyeEmotion emotion);
