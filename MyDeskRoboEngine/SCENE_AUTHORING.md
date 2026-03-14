# Scene Authoring

Diese Datei beschreibt, wie im `MyDeskRoboEngine` neue Gesichter und Emotionen gebaut werden sollen.

## Grundsatz

Nicht mit Einzelfall-Code anfangen.

Erst:
- Base
- Form
- Cuts/Brows
- Animation

Dann erst Spezialeffekte.

## EVE-Baseline

Fuer die EVE-Familie gilt:
- eine feste Grundform
- gleiche Proportionen zwischen den Emotionen
- Emotion ueber Layer-Aenderungen statt komplett neuer Gesichtsform

Das ist wichtig, damit `happy`, `sad`, `angry`, `sleepy` wie dieselbe Figur wirken.

## Szenen-Dateien

Jede Szene liegt separat unter:
- `MyDeskRoboEngine/include/scenes/*.h`

Beispiele:
- `eve_happy.h`
- `eve_sad.h`
- `eve_angry_soft.h`
- `eve_angry_hard.h`
- `eve_sleepy.h`
- `eve_glitch.h`

Eine neue Szene sollte als eigenes `EyeSceneSpec` angelegt werden.

## Empfohlener Ablauf fuer neue Emotionen

### 1. Basis waehlen
Fast immer mit `EVE Base` anfangen.

### 2. Standbild bauen
Zuerst nur die statische Form.

Typischer Aufbau:
- `Glow`
- `Mid`
- `Core`
- `Cut`
- `Brow`

### 3. Form pruefen
Wichtige Fragen:
- bleibt es klar EVE?
- stimmt die Silhouette?
- sind `cut` und `brow` ueberhaupt noetig?
- reicht ein einzelner Cut schon aus?

### 4. Erst dann Animation
Wenn die Form stimmt:
- `BlinkProfile`
- `DriftProfile`
- `PulseProfile`

### 5. Hardware-Test
Immer auf dem echten Display pruefen.

## Wann welcher Layer sinnvoll ist

### `Glow`
- fuer Leuchtkante
- sanft, nicht zu stark

### `Mid`
- stabilisiert die Form zwischen Glow und Core
- wichtig fuer den EVE-Look

### `Core`
- Hauptform des Auges
- bestimmt die Identitaet

### `Cut`
- Hintergrundfarbiger Layer zum Wegnehmen von Form
- wichtig fuer `happy`, `sad`, `angry`, `sleepy`
- sparsam einsetzen

### `Brow`
- fuer `angry`, `sad`, skeptische Varianten
- immer ueber dem Auge denken, nicht im Auge
- in der Regel gleiche Farbe wie das Auge

### Innenlayer / Pupil-Hint
- nur subtil einsetzen
- gut fuer skeptisch, fokussiert, leicht irritiert
- schnell zu viel, wenn zu dominant

## Gute Defaults fuer EVE

Normalfall:
- `Glow`, `Mid`, `Core` in Runtime-Augenfarbe
- `Cut` in Hintergrundfarbe
- `Brow` ebenfalls in Runtime-Augenfarbe

## Typische Emotionen

### `happy`
- meist unterer Cut
- keine schwere Braue
- Form freundlich, nicht nur kleiner

### `sad`
- oberer Cut mit Gegenwinkel zu `angry`
- optional weiche Braue
- nicht zu eckig werden

### `angry_soft`
- leichter oberer Cut
- oft noch ohne Braue gut

### `angry_hard`
- groesserer oberer Cut
- sichtbare Braue
- klar, aber nicht grotesk

### `sleepy`
- obere und untere Cuts
- enger Sehschlitz
- optionale `Zzz`

### `wow`
- lieber normale Basis plus Oeffnungseffekt
- nicht nur statisch groesser zeichnen

### `glitch`
- Spezialszene
- normale Basisform mit visuellen Artefakten kombinieren

## Learnings aus dem Projekt

### Rotierte LVGL-Objekte waren die falsche Abstraktion
Sobald `cut` oder `brow` schraeg wurden, wich das Ergebnis vom Editor ab.

Darum ist der Renderer jetzt canvas-basiert.

### Nicht jede Idee braucht einen neuen Stil
Oft reicht:
- gleiche Base
- anderer Cut
- andere Brow
- anderes Animationsprofil

### Labels sind okay fuer Prototypen, nicht fuer alles
`Zzz` und `?` lassen sich schnell mit Labels umsetzen.
Wenn sie aber kritisch fuer den Look sind, besser spaeter in echte Canvas-Operationen ueberfuehren.

## Was in den Szenen vermieden werden sollte

- komplett andere Base-Proportionen ohne guten Grund
- zu viele Layer, die kaum sichtbaren Mehrwert bringen
- feste Farben fuer normale EVE-Szenen
- Animationstricks, um eine schwache Form zu kaschieren

## Editor-Workflow

1. `docs/eye_designer/index.html` oeffnen
2. passendes EVE-Preset laden
3. Form bauen
4. `Renderer Spec` exportieren
5. in die passende Szenendatei uebertragen
6. Firmware bauen und auf echter Hardware pruefen

## Zielzustand

Langfristig sollte der Workflow so aussehen:
- Editor bauen
- Spec exportieren
- Szene automatisch daraus erzeugen
- flashen oder spaeter direkt laden

Bis dahin gilt:
- Editor fuer Form
- Szenendatei fuer Integration
- Hardware fuer Endpruefung
## Konverter

Fuer die Uebernahme in den `MyDeskRoboEngine` gibt es jetzt einen lokalen Konverter:
- `MyDeskRoboEngine/scripts/rendererspec_to_scene.py`

Beispiel:

```powershell
python MyDeskRoboEngine/scripts/rendererspec_to_scene.py \
  MyDeskRoboEngine/scenes/my_scene.renderer_spec.json \
  --animation kAnimIdle \
  --scene-symbol kMyScene \
  --ops-symbol kMySceneOps \
  -o MyDeskRoboEngine/include/scenes/my_scene.h
```

Danach muss die erzeugte Szene nur noch an der passenden Stelle in der Runtime referenziert werden.
