# Eye Designer

Oeffne `docs/eye_designer/index.html` direkt im Browser.

Neu:
- feste `EVE Base` entsprechend der gewaehlteten Basisform `d2_s2`
- Layer-Typen fuer `glow`, `mid`, `core`
- Preview-Formen: `square`, `round`, `rounded`
- Preview-Pixelgroesse anpassbar, standardmaessig 360x360
- separate CSS-Anzeige-Groesse zum vergroesserten Arbeiten

Funktionen:
- Layer fuer Augen und Brauen
- Presets fuer `EVE Base`, `Angry 5` und `Sad`
- Live-Vorschau
- JSON-Export
- Renderer-Spec-Export fuer verlustarme Uebernahme
- C++-Array-Snippet als Grundlage fuer den Renderer

Hinweis:
- `cut_rect` ist ein Overlay in Hintergrundfarbe zum Wegschneiden.
- `brow` ist ein rechteckiger Layer fuer Augenbrauen.
- `glow`, `mid` und `core` sind eigene semantische Layer, exportieren aber weiterhin datenorientiert.
- `EVE Base` ist jetzt die feste Ausgangsform. Fuer Emotionen sollten vor allem Layer, Brows und Cuts angepasst werden.

Renderer Spec:
- explizite Zeichenoperationen
- Farbe kann weiter an Runtime-Tuning gekoppelt bleiben
- besser geeignet als normales JSON, wenn ich deine Form 1:1 uebernehmen soll
