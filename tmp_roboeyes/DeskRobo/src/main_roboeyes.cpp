/*
 * DeskRobo — main.cpp
 * RoboEyes_LVGL demo: gleiche API wie FluxGarage Original
 */

#include <Arduino.h>
#include "Display_ST77916.h"
#include "LVGL_Driver.h"
#include "RoboEyes_LVGL.h"

RoboEyes_LVGL eyes;

// Demo sequence timing
unsigned long demoTimer = 0;
int demoStep = 0;

void setup() {
    Serial.begin(115200);

    // Init display + LVGL (your existing setup)
    Display_Init();
    LVGL_Init();

    // ── RoboEyes init ──────────────────────────────────────────────────────
    eyes.begin(
        lv_scr_act(),   // parent: active screen
        360, 360,        // display resolution
        60               // max framerate
    );

    // Color scheme: dark bg, bright cyan eyes (like the reference robot)
    eyes.setDisplayColors(
        lv_color_hex(0x04060C),   // background: near-black
        lv_color_hex(0x88DDFF)    // eye fill: light cyan
    );

    // Eye proportions — scale to 360x360 round display
    // These look great on the round screen
    eyes.setWidth(110, 110);
    eyes.setHeight(110, 110);
    eyes.setBorderradius(28, 28);
    eyes.setSpacebetween(36);

    // Animations on
    eyes.setAutoblinker(ON, 2, 4);   // auto-blink every 2-6 sec
    eyes.setIdleMode(ON, 1, 3);      // idle wander every 1-4 sec
    eyes.setCuriosity(ON);           // outer eye grows when looking sideways

    Serial.println("RoboEyes_LVGL ready");
    demoTimer = millis();
}

void loop() {
    lv_timer_handler();   // LVGL tasks
    eyes.update();        // RoboEyes frame update (rate-limited to 60fps)

    // ── Demo sequence — cycles through all moods/animations ────────────────
    unsigned long now = millis();
    if (now - demoTimer > 3000) {
        demoTimer = now;
        demoStep++;

        switch (demoStep % 12) {
            case 0:
                Serial.println("DEFAULT");
                eyes.setMood(RE_DEFAULT);
                eyes.setIdleMode(ON, 1, 3);
                break;
            case 1:
                Serial.println("HAPPY");
                eyes.setMood(RE_HAPPY);
                break;
            case 2:
                Serial.println("ANGRY");
                eyes.setMood(RE_ANGRY);
                eyes.setIdleMode(OFF);
                break;
            case 3:
                Serial.println("TIRED");
                eyes.setMood(RE_TIRED);
                break;
            case 4:
                Serial.println("CONFUSED");
                eyes.setMood(RE_DEFAULT);
                eyes.anim_confused();
                break;
            case 5:
                Serial.println("LAUGH");
                eyes.setMood(RE_HAPPY);
                eyes.anim_laugh();
                break;
            case 6:
                Serial.println("WINK");
                eyes.setMood(RE_DEFAULT);
                eyes.blink(true, false);  // left eye only
                break;
            case 7:
                Serial.println("CYCLOPS");
                eyes.setCyclops(ON);
                eyes.setMood(RE_DEFAULT);
                break;
            case 8:
                Serial.println("CYCLOPS OFF");
                eyes.setCyclops(OFF);
                break;
            case 9:
                Serial.println("SWEAT");
                eyes.setMood(RE_DEFAULT);
                eyes.setSweat(ON);
                break;
            case 10:
                Serial.println("SWEAT OFF");
                eyes.setSweat(OFF);
                break;
            case 11:
                Serial.println("CURIOUS IDLE");
                eyes.setMood(RE_DEFAULT);
                eyes.setCuriosity(ON);
                eyes.setIdleMode(ON, 1, 2);
                break;
        }
    }
}
