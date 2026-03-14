import asyncio
import queue
import threading
import tkinter as tk
from datetime import datetime
from tkinter import colorchooser, messagebox, ttk

import pc_agent as agent_runtime


BASE_EMOTIONS = [
    "IDLE",
    "HAPPY",
    "SAD",
    "ANGRY_SOFT",
    "ANGRY",
    "ANGRY_HARD",
    "WOW",
    "SLEEPY",
    "CONFUSED",
    "MAIL",
    "CALL",
    "SHAKE",
    "WINK",
    "XX",
    "GLITCH",
]
EVENTS = ["CALL", "MAIL", "TEAMS", "LOUD", "VERY_LOUD", "TILT", "SHAKE", "QUIET"]

TUNE_KEYS = [
    "drift_amp_px",
    "saccade_amp_px",
    "saccade_min_ms",
    "saccade_max_ms",
    "blink_interval_ms",
    "blink_duration_ms",
    "double_blink_chance_pct",
    "glow_pulse_amp",
    "glow_pulse_period_ms",
    "shake_amp_px",
    "shake_period_ms",
    "sleep_delay_min",
    "display_off_delay_min",
    "eye_color_r",
    "eye_color_g",
    "eye_color_b",
    "gyro_tilt_xy_pct",
    "gyro_tilt_z_pct",
    "gyro_tilt_cooldown_ms",
]

TUNE_DEFAULTS = {
    "drift_amp_px": "2",
    "saccade_amp_px": "3",
    "saccade_min_ms": "2600",
    "saccade_max_ms": "5200",
    "blink_interval_ms": "3600",
    "blink_duration_ms": "120",
    "double_blink_chance_pct": "20",
    "glow_pulse_amp": "6",
    "glow_pulse_period_ms": "2600",
    "shake_amp_px": "24",
    "shake_period_ms": "700",
    "sleep_delay_min": "10",
    "display_off_delay_min": "15",
    "eye_color_r": "15",
    "eye_color_g": "218",
    "eye_color_b": "255",
    "gyro_tilt_xy_pct": "62",
    "gyro_tilt_z_pct": "64",
    "gyro_tilt_cooldown_ms": "2200",
}


class AgentController:
    def __init__(self, event_queue: "queue.Queue[tuple]"):
        self.event_queue = event_queue
        self._thread = None
        self._loop = None
        self._stop_event = None
        self._control_queue = None
        self._lock = threading.Lock()

    def is_running(self) -> bool:
        with self._lock:
            return self._thread is not None and self._thread.is_alive()

    def start(self, mode: str) -> bool:
        with self._lock:
            if self._thread is not None and self._thread.is_alive():
                return False
            self._thread = threading.Thread(target=self._run_thread, args=(mode,), daemon=True)
            self._thread.start()
            return True

    def stop(self) -> None:
        with self._lock:
            loop = self._loop
            stop_event = self._stop_event
        if loop is not None and stop_event is not None:
            loop.call_soon_threadsafe(stop_event.set)

    def send_command(self, *cmd) -> bool:
        with self._lock:
            loop = self._loop
            control_queue = self._control_queue
        if loop is None or control_queue is None:
            return False
        try:
            loop.call_soon_threadsafe(control_queue.put_nowait, tuple(cmd))
            return True
        except Exception:
            return False

    def _run_thread(self, mode: str) -> None:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        stop_event = asyncio.Event()
        control_queue: asyncio.Queue = asyncio.Queue()

        with self._lock:
            self._loop = loop
            self._stop_event = stop_event
            self._control_queue = control_queue

        def status_callback(state: str, message: str) -> None:
            self.event_queue.put(("status", state, message))

        try:
            loop.run_until_complete(
                agent_runtime.run(
                    mode,
                    status_callback=status_callback,
                    stop_event=stop_event,
                    control_queue=control_queue,
                )
            )
            self.event_queue.put(("stopped", "", ""))
        except Exception as exc:
            self.event_queue.put(("error", "runtime", str(exc)))
        finally:
            with self._lock:
                self._loop = None
                self._stop_event = None
                self._control_queue = None
                self._thread = None
            loop.close()


class AgentApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("MyDeskRobo PC Agent")
        self.root.geometry("1020x780")
        self.root.minsize(920, 720)

        self.event_queue: "queue.Queue[tuple]" = queue.Queue()
        self.controller = AgentController(self.event_queue)

        self.mode_var = tk.StringVar(value="all")
        self.state_var = tk.StringVar(value="Nicht gestartet")
        self.detail_var = tk.StringVar(value="Der Agent verbindet sich nach dem Start automatisch per BLE.")

        self.emotion_var = tk.StringVar(value="IDLE")
        self.emotion_hold_var = tk.StringVar(value="3500")
        self.backlight_var = tk.IntVar(value=65)
        self.status_label_var = tk.BooleanVar(value=False)
        self.left_eye_var = tk.StringVar(value="IDLE")
        self.right_eye_var = tk.StringVar(value="IDLE")
        self.eye_hold_var = tk.StringVar(value="5000")
        self.raw_cmd_var = tk.StringVar(value="")

        self.tune_vars = {k: tk.StringVar(value=TUNE_DEFAULTS.get(k, "")) for k in TUNE_KEYS}
        self.eye_color_hex_var = tk.StringVar(value="#0fdaff")

        self._build_ui()
        self._update_buttons()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(200, self._poll_events)
        self.root.after(300, self._on_start)

    def _build_ui(self) -> None:
        container = ttk.Frame(self.root, padding=12)
        container.pack(fill=tk.BOTH, expand=True)

        top = ttk.Frame(container)
        top.pack(fill=tk.X)
        ttk.Label(top, text="Agent-Modus:").pack(side=tk.LEFT)
        ttk.Combobox(
            top,
            textvariable=self.mode_var,
            values=("all", "basic", "teams", "mic"),
            width=12,
            state="readonly",
        ).pack(side=tk.LEFT, padx=(8, 14))
        ttk.Button(top, text="Starten", command=self._on_start).pack(side=tk.LEFT)
        ttk.Button(top, text="Stoppen", command=self._on_stop).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Label(top, text="all = alles, basic = Outlook + Kalender").pack(side=tk.LEFT, padx=(16, 0))
        self.start_btn = top.winfo_children()[2]
        self.stop_btn = top.winfo_children()[3]

        status = ttk.LabelFrame(container, text="Verbindung", padding=12)
        status.pack(fill=tk.X, pady=(10, 10))
        ttk.Label(status, textvariable=self.state_var, font=("Segoe UI", 11, "bold")).pack(anchor="w")
        ttk.Label(status, textvariable=self.detail_var).pack(anchor="w", pady=(4, 0))

        notebook = ttk.Notebook(container)
        notebook.pack(fill=tk.BOTH, expand=True)

        control_tab = ttk.Frame(notebook, padding=10)
        tune_tab = ttk.Frame(notebook, padding=10)
        log_tab = ttk.Frame(notebook, padding=10)

        notebook.add(control_tab, text="Schnellsteuerung")
        notebook.add(tune_tab, text="Verhalten & Display")
        notebook.add(log_tab, text="Protokoll")

        self._build_control_tab(control_tab)
        self._build_tune_tab(tune_tab)
        self._build_log_tab(log_tab)
        self._refresh_emotion_options()

    def _build_control_tab(self, parent: ttk.Frame) -> None:
        hint = ttk.Label(
            parent,
            text="Schnellzugriff fuer Emotionen, Display und Test-Ereignisse. Stil ist fest auf EVE gesetzt.",
        )
        hint.pack(anchor="w", pady=(0, 8))

        look = ttk.LabelFrame(parent, text="Gesicht", padding=10)
        look.pack(fill=tk.X, pady=(0, 8))
        ttk.Label(look, text="Aktiver Stil: EVE").grid(row=0, column=0, sticky="w")
        ttk.Label(look, text="Emotion:").grid(row=1, column=0, sticky="w", pady=(10, 0))
        self.emotion_combo = ttk.Combobox(look, textvariable=self.emotion_var, state="readonly", width=18)
        self.emotion_combo.grid(row=1, column=1, sticky="w", padx=(8, 0), pady=(10, 0))
        ttk.Label(look, text="Anzeigedauer (ms):").grid(row=1, column=2, sticky="w", padx=(16, 0), pady=(10, 0))
        ttk.Entry(look, textvariable=self.emotion_hold_var, width=10).grid(row=1, column=3, sticky="w", padx=(8, 0), pady=(10, 0))
        ttk.Button(look, text="Jetzt zeigen", command=self._on_set_emotion).grid(row=1, column=4, sticky="w", padx=(12, 0), pady=(10, 0))

        eyes = ttk.LabelFrame(parent, text="Linkes / rechtes Auge", padding=10)
        eyes.pack(fill=tk.X, pady=(0, 8))
        ttk.Label(eyes, text="Links:").grid(row=0, column=0, sticky="w")
        self.left_eye_combo = ttk.Combobox(eyes, textvariable=self.left_eye_var, state="readonly", width=16)
        self.left_eye_combo.grid(row=0, column=1, sticky="w", padx=(8, 0))
        ttk.Label(eyes, text="Rechts:").grid(row=0, column=2, sticky="w", padx=(16, 0))
        self.right_eye_combo = ttk.Combobox(eyes, textvariable=self.right_eye_var, state="readonly", width=16)
        self.right_eye_combo.grid(row=0, column=3, sticky="w", padx=(8, 0))
        ttk.Label(eyes, text="Anzeigedauer (ms):").grid(row=0, column=4, sticky="w", padx=(16, 0))
        ttk.Entry(eyes, textvariable=self.eye_hold_var, width=10).grid(row=0, column=5, sticky="w", padx=(8, 0))
        ttk.Button(eyes, text="Augenpaar setzen", command=self._on_set_eyes).grid(row=0, column=6, sticky="w", padx=(12, 0))

        display = ttk.LabelFrame(parent, text="Display und Einblendungen", padding=10)
        display.pack(fill=tk.X, pady=(0, 8))
        ttk.Label(display, text="Helligkeit:").grid(row=0, column=0, sticky="w")
        ttk.Scale(display, from_=0, to=100, variable=self.backlight_var, orient="horizontal").grid(row=0, column=1, sticky="ew", padx=(8, 8))
        ttk.Label(display, textvariable=self.backlight_var, width=4).grid(row=0, column=2, sticky="w")
        ttk.Button(display, text="Helligkeit senden", command=self._on_set_backlight).grid(row=0, column=3, sticky="w", padx=(8, 0))
        ttk.Checkbutton(display, text="Technische Statuszeile unten anzeigen", variable=self.status_label_var).grid(row=1, column=0, columnspan=3, sticky="w", pady=(10, 0))
        ttk.Button(display, text="Einblendung anwenden", command=self._on_set_status_label).grid(row=1, column=3, sticky="w", padx=(8, 0), pady=(10, 0))
        display.grid_columnconfigure(1, weight=1)

        events = ttk.LabelFrame(parent, text="Ereignisse testen", padding=10)
        events.pack(fill=tk.X, pady=(0, 8))
        for i, ev in enumerate(EVENTS):
            ttk.Button(events, text=ev, command=lambda n=ev: self._send_event(n)).grid(
                row=i // 4, column=i % 4, padx=4, pady=4, sticky="ew"
            )
        for col in range(4):
            events.grid_columnconfigure(col, weight=1)

        maintenance = ttk.LabelFrame(parent, text="Wartung", padding=10)
        maintenance.pack(fill=tk.X)
        ttk.Button(maintenance, text="Uhrzeit synchronisieren", command=self._on_time_sync).grid(row=0, column=0, sticky="w")
        ttk.Button(maintenance, text="Werkseinstellungen", command=self._on_factory_reset).grid(row=0, column=1, sticky="w", padx=(8, 0))
        ttk.Label(maintenance, text="Direktbefehl (nur fuer Tests):").grid(row=1, column=0, sticky="w", pady=(12, 0))
        ttk.Entry(maintenance, textvariable=self.raw_cmd_var, width=42).grid(row=1, column=1, sticky="ew", padx=(8, 8), pady=(12, 0))
        ttk.Button(maintenance, text="Direkt senden", command=self._on_raw_cmd).grid(row=1, column=2, sticky="w", pady=(12, 0))
        maintenance.grid_columnconfigure(1, weight=1)

    def _style_emotions(self) -> list[str]:
        return list(BASE_EMOTIONS)

    def _refresh_emotion_options(self) -> None:
        emos = self._style_emotions()
        current_emo = self.emotion_var.get().strip()
        current_left = self.left_eye_var.get().strip()
        current_right = self.right_eye_var.get().strip()
        self.emotion_combo.configure(values=emos)
        self.left_eye_combo.configure(values=emos)
        self.right_eye_combo.configure(values=emos)
        self.emotion_var.set(current_emo if current_emo in emos else emos[0])
        self.left_eye_var.set(current_left if current_left in emos else emos[0])
        self.right_eye_var.set(current_right if current_right in emos else emos[0])

    def _build_tune_tab(self, parent: ttk.Frame) -> None:
        outer = ttk.Frame(parent)
        outer.pack(fill=tk.BOTH, expand=True)

        canvas = tk.Canvas(outer, highlightthickness=0)
        scrollbar = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        frame = ttk.LabelFrame(canvas, text="Verhalten und Display", padding=10)
        canvas_window = canvas.create_window((0, 0), window=frame, anchor="nw")

        def _sync_scroll_region(_event=None) -> None:
            canvas.configure(scrollregion=canvas.bbox("all"))
            canvas.itemconfigure(canvas_window, width=canvas.winfo_width())

        def _on_mousewheel(event) -> None:
            if event.delta:
                canvas.yview_scroll(int(-event.delta / 120), "units")

        frame.bind("<Configure>", _sync_scroll_region)
        canvas.bind("<Configure>", _sync_scroll_region)
        canvas.bind_all("<MouseWheel>", _on_mousewheel)

        ttk.Label(
            frame,
            text="Hier stellst du die Lebendigkeit der Augen, die Ruhezeiten und die Standardfarbe ein.",
        ).grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 10))

        top_actions = ttk.Frame(frame)
        top_actions.grid(row=1, column=0, columnspan=2, sticky="w", pady=(0, 10))
        ttk.Button(top_actions, text="Alle Werte senden", command=self._on_apply_tune).pack(side=tk.LEFT)
        ttk.Button(top_actions, text="Nur Farbe senden", command=self._on_apply_eye_color).pack(side=tk.LEFT, padx=8)
        ttk.Button(top_actions, text="Am Geraet speichern", command=self._on_save_tune).pack(side=tk.LEFT, padx=8)
        ttk.Button(top_actions, text="Vom Geraet laden", command=self._on_load_tune).pack(side=tk.LEFT)
        ttk.Button(top_actions, text="Werkseinstellungen", command=self._on_factory_reset).pack(side=tk.LEFT, padx=8)

        groups = [
            (
                "Augenbewegung",
                [
                    "drift_amp_px",
                    "saccade_amp_px",
                    "saccade_min_ms",
                    "saccade_max_ms",
                    "shake_amp_px",
                    "shake_period_ms",
                ],
            ),
            (
                "Blinken und Leuchten",
                [
                    "blink_interval_ms",
                    "blink_duration_ms",
                    "double_blink_chance_pct",
                    "glow_pulse_amp",
                    "glow_pulse_period_ms",
                ],
            ),
            (
                "Ruhezustand",
                [
                    "sleep_delay_min",
                    "display_off_delay_min",
                ],
            ),
            (
                "Augenfarbe",
                [
                    "eye_color_r",
                    "eye_color_g",
                    "eye_color_b",
                ],
            ),
            (
                "Bewegungssensor",
                [
                    "gyro_tilt_xy_pct",
                    "gyro_tilt_z_pct",
                    "gyro_tilt_cooldown_ms",
                ],
            ),
        ]

        labels = {
            "drift_amp_px": "Leichte Augenbewegung (px)",
            "saccade_amp_px": "Schnelle Blickspruenge (px)",
            "saccade_min_ms": "Pause zwischen Blickspruengen min (ms)",
            "saccade_max_ms": "Pause zwischen Blickspruengen max (ms)",
            "blink_interval_ms": "Blink alle (ms)",
            "blink_duration_ms": "Blinkdauer (ms)",
            "double_blink_chance_pct": "Doppelblink Chance (%)",
            "glow_pulse_amp": "Leuchtpuls Staerke",
            "glow_pulse_period_ms": "Leuchtpuls Tempo (ms)",
            "shake_amp_px": "Shake Bewegung (px)",
            "shake_period_ms": "Shake Tempo (ms)",
            "sleep_delay_min": "Screensaver nach (Minuten)",
            "display_off_delay_min": "Display aus nach (Minuten)",
            "eye_color_r": "Rot (0-255)",
            "eye_color_g": "Gruen (0-255)",
            "eye_color_b": "Blau (0-255)",
            "gyro_tilt_xy_pct": "Neigung XY Schwelle (%)",
            "gyro_tilt_z_pct": "Neigung Z Schwelle (%)",
            "gyro_tilt_cooldown_ms": "Neigungs-Sperre (ms)",
        }

        for idx, (title, keys) in enumerate(groups):
            group = ttk.LabelFrame(frame, text=title, padding=10)
            group.grid(row=(idx // 2) + 2, column=idx % 2, sticky="nsew", padx=6, pady=6)
            for row, key in enumerate(keys):
                ttk.Label(group, text=labels.get(key, key)).grid(row=row, column=0, sticky="w", padx=(0, 8), pady=4)
                ttk.Entry(group, textvariable=self.tune_vars[key], width=16).grid(row=row, column=1, sticky="w", pady=4)

            if title == "Augenfarbe":
                color_row = ttk.Frame(group)
                color_row.grid(row=len(keys), column=0, columnspan=2, sticky="w", pady=(8, 0))
                ttk.Button(color_row, text="Farbe waehlen", command=self._pick_eye_color).pack(side=tk.LEFT)
                ttk.Entry(color_row, textvariable=self.eye_color_hex_var, width=10).pack(side=tk.LEFT, padx=(8, 0))
                ttk.Button(color_row, text="EVE Cyan", command=lambda: self._apply_eye_color_preset("eve")).pack(side=tk.LEFT, padx=(12, 0))
                ttk.Button(color_row, text="Ice Blue", command=lambda: self._apply_eye_color_preset("ice")).pack(side=tk.LEFT, padx=(8, 0))
                ttk.Button(color_row, text="Warm White", command=lambda: self._apply_eye_color_preset("warm")).pack(side=tk.LEFT, padx=(8, 0))
                ttk.Button(color_row, text="Green Terminal", command=lambda: self._apply_eye_color_preset("terminal")).pack(side=tk.LEFT, padx=(8, 0))

        frame.columnconfigure(0, weight=1)
        frame.columnconfigure(1, weight=1)

        presets = ttk.LabelFrame(frame, text="Schnell-Presets", padding=10)
        presets.grid(row=5, column=0, columnspan=2, sticky="w", padx=6, pady=(8, 0))
        ttk.Button(presets, text="Ruhig", command=lambda: self._apply_tune_preset("calm")).pack(side=tk.LEFT)
        ttk.Button(presets, text="Standard", command=lambda: self._apply_tune_preset("balanced")).pack(side=tk.LEFT, padx=8)
        ttk.Button(presets, text="Lebhaft", command=lambda: self._apply_tune_preset("lively")).pack(side=tk.LEFT)

    def _build_log_tab(self, parent: ttk.Frame) -> None:
        self.log_text = tk.Text(parent, wrap="word", state="disabled")
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self._append_log("GUI bereit. Der Agent startet automatisch im Modus 'all'.")

    def _append_log(self, line: str) -> None:
        ts = datetime.now().strftime("%H:%M:%S")
        self.log_text.configure(state="normal")
        self.log_text.insert(tk.END, f"[{ts}] {line}\n")
        self.log_text.see(tk.END)
        self.log_text.configure(state="disabled")

    def _set_state(self, state: str, detail: str = "") -> None:
        self.state_var.set(state)
        self.detail_var.set(detail)

    def _friendly_status(self, state: str, message: str) -> tuple[str, str]:
        mapping = {
            "starting": "Startet",
            "searching": "Suche MyDeskRobo",
            "connecting": "Verbinde",
            "connected": "Verbunden",
            "disconnected": "Nicht verbunden",
            "retry": "Neuversuch",
            "heartbeat_timeout": "Verbindung instabil",
            "not_found": "MyDeskRobo nicht gefunden",
            "stopping": "Stoppt",
            "stopped": "Gestoppt",
        }
        detail = (message or "").replace("DeskRobo", "MyDeskRobo")
        return mapping.get(state, state), detail

    def _send(self, *cmd) -> bool:
        if not self.controller.is_running():
            self._append_log("Befehl nicht gesendet: Agent ist nicht gestartet.")
            return False
        ok = self.controller.send_command(*cmd)
        if not ok:
            self._append_log("Befehl konnte nicht in die Queue gelegt werden.")
        return ok

    def _on_start(self) -> None:
        mode = self.mode_var.get().strip() or "all"
        if self.controller.start(mode):
            self._set_state("Startet", f"Modus: {mode}")
            self._append_log(f"Agent gestartet (Modus: {mode}).")
        else:
            self._append_log("Agent laeuft bereits.")
        self._update_buttons()

    def _on_stop(self) -> None:
        self.controller.stop()
        self._set_state("Stoppt", "Beende Hintergrundprozess...")
        self._append_log("Stop angefordert.")
        self._update_buttons()

    def _on_set_emotion(self) -> None:
        try:
            hold = int(self.emotion_hold_var.get().strip())
        except ValueError:
            self._append_log("Die Anzeigedauer muss eine ganze Zahl sein.")
            return
        emo = self.emotion_var.get()
        if self._send("emotion", emo, hold):
            self._append_log(f"Emotion gesendet: {emo} ({hold} ms)")

    def _on_set_eyes(self) -> None:
        try:
            hold = int(self.eye_hold_var.get().strip())
        except ValueError:
            self._append_log("Die Anzeigedauer fuer das Augenpaar muss eine ganze Zahl sein.")
            return
        left = self.left_eye_var.get()
        right = self.right_eye_var.get()
        if self._send("eyes", left, right, hold):
            self._append_log(f"Augenpaar gesendet: {left}/{right} ({hold} ms)")

    def _on_set_backlight(self) -> None:
        value = int(self.backlight_var.get())
        if self._send("backlight", value):
            self._append_log(f"Display-Helligkeit gesendet: {value}%")

    def _on_set_status_label(self) -> None:
        val = bool(self.status_label_var.get())
        if self._send("status_label", val):
            self._append_log(f"Technische Statuszeile: {'AN' if val else 'AUS'}")

    def _send_event(self, name: str) -> None:
        if self._send("event", name):
            self._append_log(f"Ereignis gesendet: {name}")

    def _on_time_sync(self) -> None:
        if self._send("time_sync"):
            self._append_log("Uhrzeit-Sync gesendet.")

    def _on_factory_reset(self) -> None:
        confirmed = messagebox.askyesno(
            "Werkseinstellungen",
            "Alle gespeicherten Werte auf dem Geraet loeschen und neu starten?",
            parent=self.root,
        )
        if confirmed and self._send("factory_reset"):
            self._append_log("Werkseinstellungen angefordert. Das Geraet startet neu.")

    def _on_raw_cmd(self) -> None:
        payload = self.raw_cmd_var.get().strip()
        if not payload:
            self._append_log("Direktbefehl ist leer.")
            return
        if self._send("cmd", payload):
            self._append_log(f"Direktbefehl gesendet: {payload}")

    def _apply_tune_preset(self, name: str) -> None:
        presets = {
            "calm": {
                "drift_amp_px": 1, "saccade_amp_px": 2, "saccade_min_ms": 2200, "saccade_max_ms": 5200,
                "blink_interval_ms": 5200, "blink_duration_ms": 100, "double_blink_chance_pct": 8,
                "glow_pulse_amp": 4, "glow_pulse_period_ms": 3200, "shake_amp_px": 14, "shake_period_ms": 900,
                "sleep_delay_min": 10, "display_off_delay_min": 15,
                "gyro_tilt_xy_pct": 72, "gyro_tilt_z_pct": 74, "gyro_tilt_cooldown_ms": 2800,
            },
            "balanced": {
                "drift_amp_px": 2, "saccade_amp_px": 3, "saccade_min_ms": 2600, "saccade_max_ms": 5200,
                "blink_interval_ms": 3600, "blink_duration_ms": 120, "double_blink_chance_pct": 20,
                "glow_pulse_amp": 6, "glow_pulse_period_ms": 2600, "shake_amp_px": 24, "shake_period_ms": 700,
                "sleep_delay_min": 10, "display_off_delay_min": 15,
                "gyro_tilt_xy_pct": 62, "gyro_tilt_z_pct": 64, "gyro_tilt_cooldown_ms": 2200,
            },
            "lively": {
                "drift_amp_px": 4, "saccade_amp_px": 8, "saccade_min_ms": 900, "saccade_max_ms": 2500,
                "blink_interval_ms": 2800, "blink_duration_ms": 130, "double_blink_chance_pct": 30,
                "glow_pulse_amp": 10, "glow_pulse_period_ms": 2000, "shake_amp_px": 32, "shake_period_ms": 520,
                "sleep_delay_min": 10, "display_off_delay_min": 15,
                "gyro_tilt_xy_pct": 56, "gyro_tilt_z_pct": 58, "gyro_tilt_cooldown_ms": 1600,
            },
        }
        preset = presets.get(name)
        if not preset:
            return
        for key, value in preset.items():
            if key in self.tune_vars:
                self.tune_vars[key].set(str(value))
        self._sync_eye_color_hex_from_rgb()
        self._on_apply_tune()

    def _sync_eye_color_hex_from_rgb(self) -> None:
        try:
            r = max(0, min(255, int(self.tune_vars["eye_color_r"].get().strip())))
            g = max(0, min(255, int(self.tune_vars["eye_color_g"].get().strip())))
            b = max(0, min(255, int(self.tune_vars["eye_color_b"].get().strip())))
        except ValueError:
            return
        self.eye_color_hex_var.set(f"#{r:02x}{g:02x}{b:02x}")

    def _sync_rgb_from_eye_color_hex(self) -> None:
        raw = self.eye_color_hex_var.get().strip()
        if len(raw) != 7 or not raw.startswith("#"):
            return
        try:
            rgb = [int(raw[i:i + 2], 16) for i in (1, 3, 5)]
        except ValueError:
            return
        self.tune_vars["eye_color_r"].set(str(rgb[0]))
        self.tune_vars["eye_color_g"].set(str(rgb[1]))
        self.tune_vars["eye_color_b"].set(str(rgb[2]))

    def _pick_eye_color(self) -> None:
        self._sync_eye_color_hex_from_rgb()
        color = colorchooser.askcolor(color=self.eye_color_hex_var.get(), parent=self.root)
        if not color or not color[1]:
            return
        self.eye_color_hex_var.set(color[1])
        self._sync_rgb_from_eye_color_hex()

    def _apply_eye_color_preset(self, name: str) -> None:
        presets = {
            "eve": "#0fdaff",
            "ice": "#b8ecff",
            "warm": "#ffe6a8",
            "terminal": "#72ff9a",
        }
        value = presets.get(name)
        if not value:
            return
        self.eye_color_hex_var.set(value)
        self._sync_rgb_from_eye_color_hex()
        self._on_apply_eye_color()

    def _on_apply_eye_color(self) -> None:
        self._sync_rgb_from_eye_color_hex()
        sent = 0
        for key in ("eye_color_r", "eye_color_g", "eye_color_b"):
            raw = self.tune_vars[key].get().strip()
            if not raw:
                continue
            try:
                val = int(raw)
            except ValueError:
                self._append_log(f"Ungueltiger Wert fuer {key}: {raw}")
                continue
            if self._send("tune", key, val):
                sent += 1
        self._append_log(f"Augenfarbe gesendet: {sent} Werte")

    def _on_apply_tune(self) -> None:
        self._sync_rgb_from_eye_color_hex()
        sent = 0
        for key in TUNE_KEYS:
            raw = self.tune_vars[key].get().strip()
            if not raw:
                continue
            try:
                val = int(raw)
            except ValueError:
                self._append_log(f"Ungueltiger Wert fuer {key}: {raw}")
                continue
            if self._send("tune", key, val):
                sent += 1
        self._append_log(f"Werte gesendet: {sent}")

    def _on_save_tune(self) -> None:
        if self._send("tune_save"):
            self._append_log("Aktuelle Werte auf dem Geraet gespeichert.")

    def _on_load_tune(self) -> None:
        if self._send("tune_load"):
            self._append_log("Gespeicherte Werte auf dem Geraet aktiviert.")

    def _update_buttons(self) -> None:
        running = self.controller.is_running()
        self.start_btn.configure(state=("disabled" if running else "normal"))
        self.stop_btn.configure(state=("normal" if running else "disabled"))

    def _poll_events(self) -> None:
        while True:
            try:
                kind, state, message = self.event_queue.get_nowait()
            except queue.Empty:
                break

            if kind == "status":
                title, detail = self._friendly_status(state, message)
                self._set_state(title, detail)
                self._append_log(f"{title}: {detail}" if detail else title)
            elif kind == "error":
                self._set_state("Fehler", message)
                self._append_log(f"Fehler: {message}")
            elif kind == "stopped":
                self._set_state("Gestoppt", "")
                self._append_log("Agent wurde beendet.")

        self._update_buttons()
        self.root.after(200, self._poll_events)

    def _on_close(self) -> None:
        if self.controller.is_running():
            self.controller.stop()
        self.root.after(150, self.root.destroy)


def main() -> None:
    root = tk.Tk()
    ttk.Style().theme_use("vista")
    AgentApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
