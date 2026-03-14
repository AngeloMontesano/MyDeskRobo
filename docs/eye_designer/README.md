# Eye Designer

Oeffne `docs/eye_designer/index.html` direkt im Browser.

Der Editor ist fuer den `MyDeskRoboEngine` gedacht. Er hilft beim Bauen der Form. Die echte Endpruefung passiert immer noch auf dem Geraet.

## Zweck

Mit dem Editor baust du:
- Basisaugen
- Cuts
- Brows
- Innenlayer
- Varianten fuer Emotionen

Der Editor ist besonders nuetzlich fuer:
- `happy`
- `sad`
- `angry_soft`
- `angry_hard`
- `sleepy`
- experimentelle Varianten wie `glitch`, `wink`, `xx`

## Wichtige Regel

Nicht jede Emotion als komplett neue Augenform bauen.

Fuer EVE gilt:
- `EVE Base` laden
- dann nur Layer, Cuts und Brows aendern

So bleibt die Figur konsistent.

## Was der Editor aktuell kann

- Live-Vorschau
- Layer fuer `glow`, `mid`, `core`
- `cut`-Layer in Hintergrundfarbe
- `brow`-Layer
- `pupil`-Layer fuer subtile Innenformen
- Presets fuer vorhandene Emotionen
- Export als:
  - JSON
  - `Renderer Spec`
  - C++-Snippet

## Welcher Export ist richtig?

Fuer das Projekt ist `Renderer Spec` der richtige Export.

Warum:
- klarer als normales JSON
- naeher an der Szenenstruktur
- besser fuer verlustarme Uebernahme

## Was die Vorschau leisten kann und was nicht

Die Vorschau ist nah dran, aber nicht die finale Wahrheit.

Immer auf Hardware pruefen, wenn es um diese Punkte geht:
- Glow-Staerke
- kleine Overlays
- Fragezeichen / `Zzz`
- Opacity
- Glitch

## Layer-Logik

Typische EVE-Reihenfolge:
1. `Glow`
2. `Mid`
3. `Core`
4. `Cut`
5. `Brow`
6. optional `Pupil`

### `Cut`
- nimmt Form wieder weg
- ist in Hintergrundfarbe
- damit entstehen `happy`, `sad`, `angry`, `sleepy`

### `Brow`
- fuer starke Stimmungen
- sollte die Augen unterstuetzen, nicht verdecken

### `Pupil`
- nur subtil
- gut fuer skeptische oder fokussierte Looks

## Empfohlener Workflow

1. `EVE Base` oder passendes Emotionspreset laden
2. nur die noetigen Layer aendern
3. `Renderer Spec` exportieren
4. in die passende Szene im `MyDeskRoboEngine` uebernehmen
5. Demo flashen
6. auf echter Hardware korrigieren

## Learnings

### 1. Base zuerst festziehen
Wenn Base, Abstand oder Kernform noch schwimmen, bringt Feintuning nichts.

### 2. Cuts sind oft wichtiger als neue Grundformen
Viele gute Emotionen entstehen durch kleine, gezielte Cuts.

### 3. Brauen nur dann einsetzen, wenn sie wirklich helfen
Zu frueh eingesetzte Brauen machen EVE schnell haerter oder unfreiwillig comicartig.

### 4. Nicht ueberanimieren
Erst die Form richtig. Danach Animation.

## Dateien

Wichtige Bezugspunkte:
- `MyDeskRoboEngine/README.md`
- `MyDeskRoboEngine/SCENE_AUTHORING.md`
- `MyDeskRoboEngine/include/scenes/*.h`
