# Nextgen Scene Engine

Separater Prototyp fuer den datengetriebenen Layer-Renderer. Das aktuelle Hauptprojekt bleibt unberuehrt.

Ziel:
- Augenformen als Daten statt als harter `switch`-Code
- Runtime-Farbe weiter nutzbar
- Event- und Animationssystem spaeter darueberlegen statt wegwerfen

Inhalt:
- `include/SceneSpec.h`: Datenmodell fuer Renderer Spec
- `include/LayerRenderer.h`: generischer Renderer auf LVGL-Objekten
- `include/ScenePresets.h`: Beispielszene `EVE angry`
- `src/LayerRenderer.cpp`: Minimal-Implementierung des Layer-Renderers

Geplanter naechster Schritt:
1. Prototyp in ein eigenes kleines Demo-Target haengen
2. Blink/Pulse/Drift auf Layer-Ebene verfeinern
3. Spaeter Event-Mapping auf Specs statt hartem Emotionscode umstellen

Wichtig:
- Das ist bewusst ein neues Teilprojekt.
- Es aendert die bestehende Firmware noch nicht.
