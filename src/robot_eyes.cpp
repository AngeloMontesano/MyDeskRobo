#include "robot_eyes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
//  GLOBAL STYLES  (wie im Dokument beschrieben)
// ─────────────────────────────────────────────────────────────────────────────

static lv_style_t style_head;
static lv_style_t style_visor;
static lv_style_t style_eye_glow;    // Glow-Schatten auf Canvas-Objekt
static lv_style_t style_text;

void robot_eyes_init_styles(void)
{
    // 1. Roboterkopf
    lv_style_init(&style_head);
    lv_style_set_bg_color(&style_head,     COLOR_HEAD_BG);
    lv_style_set_bg_opa(&style_head,       LV_OPA_COVER);
    lv_style_set_border_color(&style_head, COLOR_HEAD_BORDER);
    lv_style_set_border_width(&style_head, 2);
    lv_style_set_radius(&style_head,       HEAD_RADIUS);
    lv_style_set_shadow_width(&style_head, 16);
    lv_style_set_shadow_color(&style_head, COLOR_BLACK);
    lv_style_set_shadow_opa(&style_head,   (lv_opa_t)140);
    lv_style_set_shadow_ofs_y(&style_head, 5);
    lv_style_set_pad_all(&style_head,      0);

    // 2. Visor-Recess (innere, dunklere Fläche)
    lv_style_init(&style_visor);
    lv_style_set_bg_color(&style_visor,     COLOR_VISOR_BG);
    lv_style_set_bg_opa(&style_visor,       LV_OPA_COVER);
    lv_style_set_radius(&style_visor,       VISOR_RADIUS);
    lv_style_set_border_color(&style_visor, lv_color_make(0x04, 0x06, 0x09));
    lv_style_set_border_width(&style_visor, 2);
    lv_style_set_border_opa(&style_visor,   LV_OPA_50);
    lv_style_set_pad_all(&style_visor,      0);

    // 3. Cyan-Glow auf Augen (Schatten-Effekt auf Canvas-Objekt)
    lv_style_init(&style_eye_glow);
    lv_style_set_shadow_width(&style_eye_glow,  26);
    lv_style_set_shadow_color(&style_eye_glow,  COLOR_EYE_GLOW);
    lv_style_set_shadow_spread(&style_eye_glow, 6);
    lv_style_set_shadow_opa(&style_eye_glow,    (lv_opa_t)242);

    // 4. Label-Stil
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_make(0xE8, 0xF2, 0xFF));
}

// ─────────────────────────────────────────────────────────────────────────────
//  CANVAS-ZEICHENFUNKTIONEN
//  Alle Augenformen werden per Pixel in einen lv_canvas gezeichnet,
//  damit beliebige Formen möglich sind (Halbmond, Herz, Clip).
// ─────────────────────────────────────────────────────────────────────────────

// Füllt eine horizontale Linie im Canvas
static inline void canvas_hline(lv_obj_t *cv, int x0, int x1, int y, lv_color_t col, lv_opa_t opa)
{
    if (x1 < x0) return;
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color   = col;
    d.bg_opa     = opa;
    d.radius     = 0;
    d.border_width = 0;
    lv_canvas_draw_rect(cv, x0, y, x1 - x0 + 1, 1, &d);
}

// Vollständige gefüllte Ellipse
static void fill_ellipse(lv_obj_t *cv, int cx, int cy, int rx, int ry,
                          lv_color_t col, lv_opa_t opa)
{
    for (int y = -ry; y <= ry; y++) {
        float t = (float)y / ry;
        int xw = (int)(rx * sqrtf(1.0f - t * t));
        canvas_hline(cv, cx - xw, cx + xw, cy + y, col, opa);
    }
}

// Untere Hälfte der Ellipse (Happy-Bogen)
static void fill_ellipse_bottom(lv_obj_t *cv, int cx, int cy, int rx, int ry,
                                 lv_color_t col, lv_opa_t opa)
{
    for (int y = 0; y <= ry; y++) {
        float t = (float)y / ry;
        int xw = (int)(rx * sqrtf(1.0f - t * t));
        canvas_hline(cv, cx - xw, cx + xw, cy + y, col, opa);
    }
}

// Ellipse mit Scherung (SAD-Kippung)
// shear: Pixel, um die die Oberseite nach rechts verschoben wird (neg = links)
static void fill_ellipse_shear(lv_obj_t *cv, int cx, int cy, int rx, int ry,
                                lv_color_t col, lv_opa_t opa, int shear)
{
    for (int y = -ry; y <= ry; y++) {
        float t = (float)y / ry;
        int xw = (int)(rx * sqrtf(1.0f - t * t));
        // Scherung: lineare Verschiebung basierend auf y
        int shift = (int)(shear * (-t)); // oben = max shift, unten = -max
        canvas_hline(cv, cx - xw + shift, cx + xw + shift, cy + y, col, opa);
    }
}

// Ellipse mit diagonalem Clip in der inneren oberen Ecke (ANGRY)
// side: 0 = linkes Auge (clip rechts-oben-innen), 1 = rechtes Auge (clip links-oben-innen)
static void fill_ellipse_angry(lv_obj_t *cv, int cx, int cy, int rx, int ry,
                                lv_color_t col, lv_opa_t opa, int side)
{
    for (int y = -ry; y <= ry; y++) {
        float t = (float)y / ry;  // -1..+1
        int xw = (int)(rx * sqrtf(1.0f - t * t));
        int x0 = cx - xw;
        int x1 = cx + xw;

        // Nur obere Hälfte clippen
        if (y < 0) {
            // Clip-Linie läuft von (cx, 0) diagonal zur inneren oberen Ecke
            // Bei side=0 (links): innere Seite = rechts → clip x1
            // Bei side=1 (rechts): innere Seite = links → clip x0
            float frac = (float)(-y) / ry;  // 0 an Mitte, 1 an Spitze
            int clip_amt = (int)(xw * frac * 0.80f);
            if (side == 0) {
                x1 = cx + xw - clip_amt;
            } else {
                x0 = cx - xw + clip_amt;
            }
        }
        if (x1 > x0) canvas_hline(cv, x0, x1, cy + y, col, opa);
    }
}

// Herz-Form
static void fill_heart(lv_obj_t *cv, int cx, int cy, int rx, int ry,
                        lv_color_t col, lv_opa_t opa)
{
    float bump_r  = rx * 0.54f;
    float bump_ox = rx * 0.28f;
    float bump_cy_off = ry * 0.18f;  // bump-Zentren leicht oberhalb der Mitte

    for (int y = -ry; y <= ry; y++) {
        float t = (float)y / ry;   // -1 (oben) .. +1 (unten)
        int xw = 0;

        if (t <= 0) {
            // Obere Hälfte: Union zweier Kreise
            float dy   = (float)y + bump_cy_off * ry / ry;  // Abstand von Bump-Zentrum
            float r2   = bump_r * bump_r - dy * dy;
            float bx   = (r2 > 0) ? sqrtf(r2) : 0.0f;
            float left  = -bump_ox - bx;
            float right =  bump_ox + bx;
            if (right > left) {
                int xl = cx + (int)left;
                int xr = cx + (int)right;
                canvas_hline(cv, xl, xr, cy + y, col, opa);
            }
            continue;
        } else {
            // Untere Hälfte: spitz zulaufend
            xw = (int)(rx * (1.0f - t) * 1.05f);
        }
        if (xw > 0) canvas_hline(cv, cx - xw, cx + xw, cy + y, col, opa);
    }
}

// Highlight-Punkte auf dem Auge (Spiegelung)
static void draw_eye_highlight(lv_obj_t *cv, int cx, int cy, int rx, int ry)
{
    // Großer weicher Highlight oben-links
    int hx = cx - (int)(rx * 0.16f);
    int hy = cy - (int)(ry * 0.26f);
    fill_ellipse(cv, hx, hy, (int)(rx * 0.34f), (int)(ry * 0.22f),
                 lv_color_white(), LV_OPA_70);

    // Kleiner Highlight-Punkt
    fill_ellipse(cv, cx + (int)(rx * 0.22f), cy - (int)(ry * 0.48f),
                 (int)(rx * 0.14f), (int)(ry * 0.10f),
                 lv_color_white(), LV_OPA_50);
}

// ─────────────────────────────────────────────────────────────────────────────
//  EIN AUGE IN CANVAS ZEICHNEN
// ─────────────────────────────────────────────────────────────────────────────

typedef enum {
    SHAPE_OVAL,
    SHAPE_ARC_BOTTOM,
    SHAPE_SAD_L,
    SHAPE_SAD_R,
    SHAPE_ANGRY_L,
    SHAPE_ANGRY_R,
    SHAPE_SLIT,
    SHAPE_HEART,
} EyeShape;

// w, h: Augen-Radius in Pixeln (passt sich an Canvas-Mitte an)
static void draw_eye(lv_obj_t *cv, EyeShape shape, int w, int h)
{
    int cx = EYE_CANVAS_W / 2;
    int cy = EYE_CANVAS_H / 2;
    int rx = w / 2;
    int ry = h / 2;

    // Hintergrund = Visor-Farbe
    lv_canvas_fill_bg(cv, COLOR_VISOR_BG, LV_OPA_COVER);

    lv_color_t glow_col = COLOR_EYE_GLOW;
    lv_color_t core_col = COLOR_EYE_CORE;

    auto draw_glow_ellipse = [&](int ex, int ey, int erx, int ery) {
        fill_ellipse(cv, ex, ey, erx + 5, ery + 5, glow_col, (lv_opa_t)90);
        fill_ellipse(cv, ex, ey, erx + 2, ery + 2, glow_col, (lv_opa_t)140);
        fill_ellipse(cv, ex, ey, erx, ery, core_col, LV_OPA_COVER);
    };

    switch (shape) {

        case SHAPE_OVAL:
            draw_glow_ellipse(cx, cy, rx, ry);
            draw_eye_highlight(cv, cx, cy, rx, ry);
            break;

        case SHAPE_ARC_BOTTOM:
            // Crescent style to match the reference happy arc.
            fill_ellipse_bottom(cv, cx, cy - ry / 3, rx + 2, ry + 3, glow_col, (lv_opa_t)160);
            fill_ellipse_bottom(cv, cx, cy - ry / 3, rx, ry + 1, core_col, LV_OPA_COVER);
            fill_ellipse_bottom(cv, cx, cy - ry / 3 + 4, rx - 2, ry - 2, COLOR_VISOR_BG, LV_OPA_COVER);
            fill_ellipse(cv, cx - rx / 5, cy - ry / 3 - 1,
                         rx / 4, ry / 6, lv_color_white(), (lv_opa_t)115);
            break;

        case SHAPE_SAD_L:
            fill_ellipse_shear(cv, cx, cy, rx + 2, ry + 2, glow_col, (lv_opa_t)150, +8);
            fill_ellipse_shear(cv, cx, cy, rx, ry, core_col, LV_OPA_COVER, +7);
            draw_eye_highlight(cv, cx, cy, rx, ry);
            break;

        case SHAPE_SAD_R:
            fill_ellipse_shear(cv, cx, cy, rx + 2, ry + 2, glow_col, (lv_opa_t)150, -8);
            fill_ellipse_shear(cv, cx, cy, rx, ry, core_col, LV_OPA_COVER, -7);
            draw_eye_highlight(cv, cx, cy, rx, ry);
            break;

        case SHAPE_ANGRY_L:
            fill_ellipse_angry(cv, cx, cy, rx + 2, ry + 2, glow_col, (lv_opa_t)160, 0);
            fill_ellipse_angry(cv, cx, cy, rx, ry, core_col, LV_OPA_COVER, 0);
            fill_ellipse(cv, cx - rx / 4, cy + ry / 6,
                         rx / 3, ry / 5, lv_color_white(), LV_OPA_40);
            break;

        case SHAPE_ANGRY_R:
            fill_ellipse_angry(cv, cx, cy, rx + 2, ry + 2, glow_col, (lv_opa_t)160, 1);
            fill_ellipse_angry(cv, cx, cy, rx, ry, core_col, LV_OPA_COVER, 1);
            fill_ellipse(cv, cx + rx / 4, cy + ry / 6,
                         rx / 3, ry / 5, lv_color_white(), LV_OPA_40);
            break;

        case SHAPE_SLIT:
            draw_glow_ellipse(cx, cy, rx, ry);
            fill_ellipse(cv, cx - rx / 5, cy, (int)(rx * 0.52f), (int)(ry * 0.6f),
                         lv_color_white(), LV_OPA_40);
            break;

        case SHAPE_HEART:
            fill_heart(cv, cx, cy, rx + 2, ry + 2, glow_col, (lv_opa_t)150);
            fill_heart(cv, cx, cy, rx, ry, core_col, LV_OPA_COVER);
            fill_ellipse(cv, cx - rx / 4, cy - ry / 3,
                         rx / 4, ry / 6, lv_color_white(), (lv_opa_t)166);
            fill_ellipse(cv, cx + rx / 5, cy - ry / 2,
                         rx / 7, ry / 8, lv_color_white(), (lv_opa_t)115);
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  EMOTIONSKONFIGURATION
// ─────────────────────────────────────────────────────────────────────────────

typedef struct {
    const char *label;
    EyeShape    left_shape;
    EyeShape    right_shape;
    int         eye_w;      // Breite des Augen-Ovals
    int         eye_h;      // Höhe des Augen-Ovals
    int         left_w;     // Override links (0 = eye_w)
    int         left_h;
    int         right_w;
    int         right_h;
    int         left_dy;    // Y-Versatz des linken Auges
    int         right_dy;
} EmotionDef;

static const EmotionDef EMOTION_TABLE[EYE_COUNT] = {
    // Emotion       label          left_shape      right_shape    ew  eh  lw  lh  rw  rh  ldy rdy
    [EYE_IDLE]      = {"1. IDLE",      SHAPE_OVAL,       SHAPE_OVAL,       22, 36,  0,  0,  0,  0,   0,  0},
    [EYE_HAPPY]     = {"2. HAPPY",     SHAPE_ARC_BOTTOM, SHAPE_ARC_BOTTOM, 30, 24,  0,  0,  0,  0,   4,  4},
    [EYE_SAD]       = {"3. SAD",       SHAPE_SAD_L,      SHAPE_SAD_R,      20, 30,  0,  0,  0,  0,  -2, -2},
    [EYE_ANGRY]     = {"4. ANGRY",     SHAPE_ANGRY_L,    SHAPE_ANGRY_R,    28, 20,  0,  0,  0,  0,  -3, -3},
    [EYE_SURPRISED] = {"5. SURPRISED", SHAPE_OVAL,       SHAPE_OVAL,       26, 40,  0,  0,  0,  0,   0,  0},
    [EYE_SLEEPY]    = {"6. SLEEPY",    SHAPE_SLIT,       SHAPE_SLIT,       30,  7,  0,  0,  0,  0,   5,  5},
    [EYE_WINK]      = {"7. WINK",      SHAPE_ARC_BOTTOM, SHAPE_OVAL,       26, 18,  0,  0, 22, 36,   4,  0},
    [EYE_LOVE]      = {"8. LOVE",      SHAPE_HEART,      SHAPE_HEART,      24, 28,  0,  0,  0,  0,   0,  0},
};

// ─────────────────────────────────────────────────────────────────────────────
//  HILFSFUNKTION: Kopf + Visor + zwei Augen-Canvas anlegen
// ─────────────────────────────────────────────────────────────────────────────

// Canvas-Puffer aus PSRAM
#define EYE_BUF_BYTES  LV_CANVAS_BUF_SIZE_TRUE_COLOR(EYE_CANVAS_W, EYE_CANVAS_H)

static void create_robot_head(lv_obj_t *parent,
                               lv_coord_t x, lv_coord_t y,
                               EyeEmotion emotion,
                               lv_color_t *buf_l, lv_color_t *buf_r)
{
    const EmotionDef *e = &EMOTION_TABLE[emotion];

    // ── Kopf ─────────────────────────────────────────────────────────────────
    lv_obj_t *head = lv_obj_create(parent);
    lv_obj_add_style(head, &style_head, 0);
    lv_obj_set_size(head, HEAD_W, HEAD_H);
    lv_obj_set_pos(head, x, y);
    lv_obj_clear_flag(head, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(head, LV_SCROLLBAR_MODE_OFF);

    // ── Visor ────────────────────────────────────────────────────────────────
    lv_coord_t vw = HEAD_W - VISOR_PAD * 2;
    lv_coord_t vh = HEAD_H - VISOR_PAD * 2;
    lv_obj_t *visor = lv_obj_create(head);
    lv_obj_add_style(visor, &style_visor, 0);
    lv_obj_set_size(visor, vw, vh);
    lv_obj_align(visor, LV_ALIGN_CENTER, 0, 1);
    lv_obj_clear_flag(visor, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(visor, LV_SCROLLBAR_MODE_OFF);

    // ── Augen-Größen ─────────────────────────────────────────────────────────
    int lw = e->left_w  ? e->left_w  : e->eye_w;
    int lh = e->left_h  ? e->left_h  : e->eye_h;
    int rw = e->right_w ? e->right_w : e->eye_w;
    int rh = e->right_h ? e->right_h : e->eye_h;

    // ── Linkes Auge ──────────────────────────────────────────────────────────
    lv_obj_t *cv_l = lv_canvas_create(visor);
    lv_canvas_set_buffer(cv_l, buf_l, EYE_CANVAS_W, EYE_CANVAS_H, LV_IMG_CF_TRUE_COLOR);
    draw_eye(cv_l, e->left_shape, lw, lh);

    int total_w  = EYE_CANVAS_W * 2 + EYE_SPACING;
    int start_x  = (vw - total_w) / 2;
    lv_obj_set_pos(cv_l, start_x, (vh - EYE_CANVAS_H) / 2 + e->left_dy);
    lv_obj_add_style(cv_l, &style_eye_glow, 0);

    // ── Rechtes Auge ─────────────────────────────────────────────────────────
    lv_obj_t *cv_r = lv_canvas_create(visor);
    lv_canvas_set_buffer(cv_r, buf_r, EYE_CANVAS_W, EYE_CANVAS_H, LV_IMG_CF_TRUE_COLOR);
    draw_eye(cv_r, e->right_shape, rw, rh);

    lv_obj_set_pos(cv_r, start_x + EYE_CANVAS_W + EYE_SPACING,
                   (vh - EYE_CANVAS_H) / 2 + e->right_dy);
    lv_obj_add_style(cv_r, &style_eye_glow, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PUBLIC: 4×2 RASTER — direkte Umsetzung des Dokuments
// ─────────────────────────────────────────────────────────────────────────────

void robot_eyes_grid_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_make(0x02, 0x06, 0x0B), 0);
    lv_obj_set_style_bg_grad_color(parent, lv_color_make(0x06, 0x16, 0x22), 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(parent,  LV_OPA_COVER, 0);

    // Gitter-Definitionen (4 Spalten, 2 Zeilen) — wie im Dokument
    static lv_coord_t col_dsc[] = {
        HEAD_W + 30, HEAD_W + 30, HEAD_W + 30, HEAD_W + 30,
        LV_GRID_TEMPLATE_LAST
    };
    static lv_coord_t row_dsc[] = {
        HEAD_H + 50, HEAD_H + 50,
        LV_GRID_TEMPLATE_LAST
    };

    // Container (wie im Dokument)
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_size(cont, LV_PCT(98), LV_PCT(98));
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont,     LV_OPA_0, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(cont,    0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    static lv_color_t eye_bufs[EYE_COUNT][2][EYE_CANVAS_W * EYE_CANVAS_H];

    for (int i = 0; i < EYE_COUNT; i++) {
        int row = i / 4;
        int col = i % 4;
        const EmotionDef *e = &EMOTION_TABLE[i];

        // ── Kopf im Grid ──────────────────────────────────────────────────────
        lv_obj_t *head = lv_obj_create(cont);
        lv_obj_add_style(head, &style_head, 0);
        lv_obj_set_size(head, HEAD_W, HEAD_H);
        lv_obj_set_grid_cell(head,
            LV_GRID_ALIGN_CENTER, col, 1,
            LV_GRID_ALIGN_CENTER, row, 1);
        lv_obj_clear_flag(head, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(head, LV_SCROLLBAR_MODE_OFF);

        // ── Visor ─────────────────────────────────────────────────────────────
        lv_coord_t vw = HEAD_W - VISOR_PAD * 2;
        lv_coord_t vh = HEAD_H - VISOR_PAD * 2;
        lv_obj_t *visor = lv_obj_create(head);
        lv_obj_add_style(visor, &style_visor, 0);
        lv_obj_set_size(visor, vw, vh);
        lv_obj_align(visor, LV_ALIGN_CENTER, 0, 1);
        lv_obj_clear_flag(visor, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(visor, LV_SCROLLBAR_MODE_OFF);

        // ── Augen ─────────────────────────────────────────────────────────────
        int lw = e->left_w  ? e->left_w  : e->eye_w;
        int lh = e->left_h  ? e->left_h  : e->eye_h;
        int rw = e->right_w ? e->right_w : e->eye_w;
        int rh = e->right_h ? e->right_h : e->eye_h;

        int total_w = EYE_CANVAS_W * 2 + EYE_SPACING;
        int start_x = (vw - total_w) / 2;

        lv_obj_t *cv_l = lv_canvas_create(visor);
        lv_canvas_set_buffer(cv_l, eye_bufs[i][0],
                              EYE_CANVAS_W, EYE_CANVAS_H, LV_IMG_CF_TRUE_COLOR);
        draw_eye(cv_l, e->left_shape, lw, lh);
        lv_obj_set_pos(cv_l, start_x, (vh - EYE_CANVAS_H) / 2 + e->left_dy);
        lv_obj_add_style(cv_l, &style_eye_glow, 0);

        lv_obj_t *cv_r = lv_canvas_create(visor);
        lv_canvas_set_buffer(cv_r, eye_bufs[i][1],
                              EYE_CANVAS_W, EYE_CANVAS_H, LV_IMG_CF_TRUE_COLOR);
        draw_eye(cv_r, e->right_shape, rw, rh);
        lv_obj_set_pos(cv_r, start_x + EYE_CANVAS_W + EYE_SPACING,
                       (vh - EYE_CANVAS_H) / 2 + e->right_dy);
        lv_obj_add_style(cv_r, &style_eye_glow, 0);

        // ── Label (wie im Dokument: gleiche Gitterzelle, ALIGN_BOTTOM_CENTER) ─
        lv_obj_t *label = lv_label_create(cont);
        lv_label_set_text(label, e->label);
        lv_obj_add_style(label, &style_text, 0);
        lv_obj_set_grid_cell(label,
            LV_GRID_ALIGN_CENTER, col, 1,
            LV_GRID_ALIGN_END,    row, 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PUBLIC: Einzelner großer Gesichts-Modus mit robot_eyes_set_emotion()
// ─────────────────────────────────────────────────────────────────────────────

// Für den Single-Face-Modus hält diese Struktur die Canvas-Referenzen
static struct {
    lv_obj_t   *cv_l, *cv_r;
    lv_color_t *buf_l, *buf_r;
    lv_coord_t  vw, vh;
    EyeEmotion  current;
    bool        initialized;
} s_face = {0};

#define BIG_SCALE     3          // Skalierungsfaktor gegenüber Grid-Größe
#define BIG_HEAD_W    (HEAD_W  * BIG_SCALE / 2 * 2)   // ~210
#define BIG_HEAD_H    (HEAD_H  * BIG_SCALE / 2 * 2)   // ~150
#define BIG_EYE_CW    (EYE_CANVAS_W * BIG_SCALE / 2)  // ~54
#define BIG_EYE_CH    (EYE_CANVAS_H * BIG_SCALE / 2)  // ~78
#define BIG_EYE_GAP   24

lv_obj_t* robot_eyes_create_face(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, COLOR_BACKGROUND, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *head = lv_obj_create(parent);
    lv_obj_add_style(head, &style_head, 0);
    lv_obj_set_size(head, BIG_HEAD_W, BIG_HEAD_H);
    lv_obj_align(head, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(head, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(head, LV_SCROLLBAR_MODE_OFF);

    lv_coord_t vw = BIG_HEAD_W - VISOR_PAD * 2;
    lv_coord_t vh = BIG_HEAD_H - VISOR_PAD * 2;
    s_face.vw = vw;
    s_face.vh = vh;

    lv_obj_t *visor = lv_obj_create(head);
    lv_obj_add_style(visor, &style_visor, 0);
    lv_obj_set_size(visor, vw, vh);
    lv_obj_align(visor, LV_ALIGN_CENTER, 0, 2);
    lv_obj_clear_flag(visor, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(visor, LV_SCROLLBAR_MODE_OFF);

    size_t buf_sz = LV_CANVAS_BUF_SIZE_TRUE_COLOR(BIG_EYE_CW, BIG_EYE_CH);
    s_face.buf_l  = (lv_color_t*)malloc(buf_sz);
    s_face.buf_r  = (lv_color_t*)malloc(buf_sz);

    int start_x = (vw - (BIG_EYE_CW * 2 + BIG_EYE_GAP)) / 2;
    int eye_y   = (vh - BIG_EYE_CH) / 2;

    s_face.cv_l = lv_canvas_create(visor);
    lv_canvas_set_buffer(s_face.cv_l, s_face.buf_l,
                          BIG_EYE_CW, BIG_EYE_CH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(s_face.cv_l, start_x, eye_y);
    lv_obj_add_style(s_face.cv_l, &style_eye_glow, 0);

    s_face.cv_r = lv_canvas_create(visor);
    lv_canvas_set_buffer(s_face.cv_r, s_face.buf_r,
                          BIG_EYE_CW, BIG_EYE_CH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_pos(s_face.cv_r, start_x + BIG_EYE_CW + BIG_EYE_GAP, eye_y);
    lv_obj_add_style(s_face.cv_r, &style_eye_glow, 0);

    s_face.initialized = true;
    robot_eyes_set_emotion(EYE_IDLE);

    return head;
}

void robot_eyes_set_emotion(EyeEmotion emotion)
{
    if (!s_face.initialized || emotion >= EYE_COUNT) return;
    const EmotionDef *e = &EMOTION_TABLE[emotion];

    // Skalierte Augen-Maße
    int lw = (e->left_w  ? e->left_w  : e->eye_w) * BIG_EYE_CW / EYE_CANVAS_W;
    int lh = (e->left_h  ? e->left_h  : e->eye_h) * BIG_EYE_CH / EYE_CANVAS_H;
    int rw = (e->right_w ? e->right_w : e->eye_w) * BIG_EYE_CW / EYE_CANVAS_W;
    int rh = (e->right_h ? e->right_h : e->eye_h) * BIG_EYE_CH / EYE_CANVAS_H;

    // Y-Versatz anwenden
    lv_coord_t base_y = (s_face.vh - BIG_EYE_CH) / 2;
    lv_obj_set_y(s_face.cv_l, base_y + e->left_dy  * BIG_SCALE / 2);
    lv_obj_set_y(s_face.cv_r, base_y + e->right_dy * BIG_SCALE / 2);

    draw_eye(s_face.cv_l, e->left_shape,  lw, lh);
    draw_eye(s_face.cv_r, e->right_shape, rw, rh);

    lv_obj_invalidate(s_face.cv_l);
    lv_obj_invalidate(s_face.cv_r);

    s_face.current = emotion;
}
