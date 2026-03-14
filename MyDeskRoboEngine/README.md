# MyDeskRoboEngine

Separater datengetriebener Face-Renderer fuer MyDeskRobo.

`MyDeskRoboEngine` ist der aktive Unterbau fuer Gesichter und Emotionen, die sauber als Daten statt als harter Sonderfall-Code gebaut werden sollen.

## Ziel

- Gesichter als Szenendateien definieren
- Animation von der Form trennen
- Augenfarbe weiter zur Laufzeit aenderbar halten
- Emotionen spaeter ueber Web, BLE oder PC-Agent schaltbar machen
- Formen moeglichst 1:1 aus dem Eye Designer uebernehmen

## Status

Der wichtige Architekturwechsel ist bereits passiert:

- Renderer arbeitet canvas-basiert
- `cut` und `brow` werden nicht mehr ueber rotierte LVGL-Objekte gebaut
- dadurch stimmen schräge Cuts/Brauen deutlich besser mit dem Editor ueberein

Das war der entscheidende Fix. Der fruehere Objektansatz war fuer einfache Base-Augen okay, aber bei Emotionen mit schraegen Overlays unzuverlaessig.

## Struktur

- `include/SceneSpec.h`
  - Datenmodell fuer Szenen, Layer und Animation
- `include/LayerRenderer.h`
  - Renderer-Schnittstelle
- `src/LayerRenderer.cpp`
  - Canvas-basierter Renderer
- `include/scenes/*.h`
  - einzelne Szenen/Emotionen

## Kernbegriffe

### `EyeSceneSpec`
Beschreibt eine komplette Szene:
- Name
- Geometrie
- Hintergrundfarbe
- Animationsprofil
- Liste von Render-Operationen

### `RenderOp`
Ein Layer bzw. eine Zeichenoperation.
Wichtige Felder:
- `op`: was gezeichnet wird (`DrawGlow`, `DrawMid`, `DrawCore`, `DrawCut`, `DrawBrow`, `DrawShape`)
- `primitive`: Form (`Ellipse`, `RoundedRect`)
- `side`: links, rechts oder beide
- `offset`: Position relativ zur Basis
- `size`: Breite/Hoehe
- `angle_deg`: Winkel
- `color`: Laufzeit-Augenfarbe, Hintergrund oder feste Farbe
- `opacity`: Deckkraft

### `AnimationProfile`
Trennt Bewegung von der Form.
Enthaelt:
- `BlinkProfile`
- `DriftProfile`
- `PulseProfile`

### `RuntimeState`
Laufzeitwerte, die der Renderer pro Frame bekommt:
- aktuelle Augenfarbe
- Blinkzustand
- Drift/Sakkaden
- Pulse
- Glitch-Zusatzdaten

## Wie eine Szene gebaut wird

Die Basisidee ist:

1. feste Grundform definieren
2. Emotion nur ueber Zusatzlayer/Cuts/Brows ableiten
3. Animation separat festlegen

Fuer EVE bedeutet das praktisch:
- die Base-Proportionen bleiben konsistent
- Emotionen aendern vor allem `cut`, `brow`, evtl. Innenlayer
- dadurch bleibt es dieselbe Figur und nicht jedes Mal ein anderes Gesicht

## Aktueller EVE-Workflow

1. `EVE Base` im Eye Designer laden
2. Emotion als Abwandlung davon bauen
3. `Renderer Spec` exportieren
4. Werte in die passende Szenendatei uebernehmen
5. Firmware flashen und auf echter Hardware pruefen

Wichtig:
- erst Form richtig machen
- danach Bewegung feinjustieren
- nicht umgekehrt

## Empfohlene Reihenfolge fuer neue Emotionen

1. Base festlegen
2. Szene als Standbild bauen
3. `cut`/`brow` pruefen
4. Animation hinzufuegen
5. erst danach Spezialeffekte

Wenn die Form nicht stimmt, ist jedes weitere Tuning verschwendet.

## Aktuelle Learnings

### 1. LVGL-Objekte reichen fuer komplexe Cuts/Brows nicht aus
Der alte Versuch, jeden Layer als eigenes LVGL-Objekt zu rotieren, war fuer `angry`/`sad` nicht stabil genug.

Folge:
- `cut` sichtbar in einem Stil, im anderen nicht
- Brauen verschwanden oder sassen falsch
- Editor und Geraet sahen nicht gleich aus

Loesung:
- Canvas-basierter Renderer fuer die Szene
- Layer in fester Reihenfolge direkt rendern

### 2. `Base` kann stimmen, Emotion trotzdem nicht
Die reine Grundform kann gut sein und trotzdem passt die Emotion nicht.

Darum:
- Base nur einmal festziehen
- Emotionen nur ueber Overlays veraendern

### 3. Form und Bewegung muessen getrennt bleiben
Wenn Bewegung in die Szene “eingebacken” wird, verliert man die Kontrolle.

Darum:
- Szene beschreibt nur die Form
- Animation beschreibt nur Verhalten

### 4. Auf echter Hardware pruefen
Ein Editor ist nur Vorschau.
Gerade bei:
- Glow
- Opacity
- kleinen Text-/Overlay-Elementen
- Glitch
muss immer auf dem realen Display geprueft werden.

### 5. Labels sind fuer kleine Effekte fragil
`Zzz` und `?` ueber LVGL-Labels funktionieren, sind aber empfindlicher als echte Canvas-Formen.
Wenn etwas dauerhaft wichtig ist, ist ein echter gerenderter Layer robuster.

## Layer-Regeln

### Reihenfolge
Typische EVE-Reihenfolge:
1. `Glow`
2. `Mid`
3. `Core`
4. `Cut`
5. `Brow`
6. optionale Innenlayer

`Cut` ist Hintergrundfarbe und nimmt wieder Form weg.

### Farbe
Fuer EVE normalerweise:
- `Glow`, `Mid`, `Core`, `Brow`: `RuntimeEye`
- `Cut`: `SceneBackground`

Das erlaubt weiterhin Farbwechsel per Web/GUI.

### Mirror-Regeln
Bei zweiseitigen Layern immer bewusst festlegen:
- `mirror_x_for_right`
- `mirror_angle_for_right`

Das ist bei `angry` und `sad` entscheidend.

## Was eine gute EVE-Szene ausmacht

- gleiche Base-Proportionen wie die anderen EVE-Szenen
- Emotion vor allem ueber Cuts und Brauen
- keine unnoetig neue Grundform
- runtime-gesteuerte Augenfarbe
- sparsame Spezialeffekte

## Glitch

`glitch` ist bewusst eine Spezialszene.
Sie kombiniert:
- normale Augenform
- zusaetzliche Rot-/Cyan-Versatzebenen
- Row-Shift
- Scanlines
- leichtes Flicker

Wichtig:
- weniger Flicker wirkt meist besser als zu viel
- Scanlines tragen die Szene staerker als Opacity-Flackern

## Firmware builden und flashen

Build:

```powershell
$env:PYTHONIOENCODING='utf-8'; $env:PYTHONUTF8='1'; chcp 65001 > $null; & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full
```

Upload:

```powershell
$env:PYTHONIOENCODING='utf-8'; $env:PYTHONUTF8='1'; chcp 65001 > $null; & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-mydeskrobo-full -t upload
```

## Nächste saubere Ausbaustufen

1. `Renderer Spec <-> Szene` Konverter
2. mehr Effekte direkt im Canvas statt als Labels
3. spaeter Import ueber Web/BLE statt jedes Mal neu zu flashen

## Referenz

Fuer das konkrete Bauen neuer Gesichter und Emotionen siehe auch:
- `MyDeskRoboEngine/SCENE_AUTHORING.md`
- `docs/eye_designer/README.md`
## Renderer-Spec-zu-Scene-Konverter

Es gibt jetzt einen lokalen Konverter:
- `MyDeskRoboEngine/scripts/rendererspec_to_scene.py`

Er nimmt einen `Renderer Spec` aus dem Eye Designer und erzeugt daraus direkt eine `Scene .h` fuer den `MyDeskRoboEngine`.

### Nutzen

Damit entfaellt die manuelle Uebertragung vom Editor-Export nach C++.

Workflow:
1. im Eye Designer bauen
2. `Renderer Spec` exportieren
3. Konverter laufen lassen
4. `.h` in `include/scenes/` ablegen
5. in den Runtime-Code einbinden
6. builden/flashen

### Beispiel

```powershell
python MyDeskRoboEngine/scripts/rendererspec_to_scene.py \
  MyDeskRoboEngine/scenes/eve_angry_soft.renderer_spec.json \
  --animation kAnimAlert \
  --scene-symbol kEveAngrySoftScene \
  --ops-symbol kEveAngrySoftOps \
  -o MyDeskRoboEngine/include/scenes/eve_angry_soft.h
```

### Parameter

- `spec`
  - Eingabe-JSON im `Renderer Spec`-Format
- `--animation`
  - welches Animationsprofil verwendet werden soll, z. B. `kAnimIdle`, `kAnimAlert`, `kAnimSleepy`
- `--scene-symbol`
  - optionaler Name der `EyeSceneSpec`-Variable
- `--ops-symbol`
  - optionaler Name des `RenderOp`-Arrays
- `-o`, `--output`
  - Zielpfad der `.h`-Datei

### Verhalten

Der Konverter uebernimmt:
- Geometrie
- Hintergrundfarbe
- alle Operationen
- Seiten-/Mirror-Flags
- Farben
- Opacity

Animation bleibt bewusst ein eigener Parameter. Das ist Absicht, weil Form und Verhalten getrennt bleiben sollen.

### Hinweise

- JSON mit BOM wird ebenfalls gelesen
- fuer normale EVE-Szenen sollte `cut` weiter `SceneBackground` nutzen
- fuer normale EVE-Szenen sollte `Glow/Mid/Core/Brow` weiter `RuntimeEye` nutzen
