import asyncio
import queue
import threading
import tkinter as tk
from datetime import datetime
from tkinter import ttk

import pc_agent as agent_runtime


EMOTIONS = [
    "IDLE",
    "HAPPY",
    "SAD",
    "ANGRY",
    "WOW",
    "SLEEPY",
    "CONFUSED",
    "EXCITED",
    "DIZZY",
    "MAIL",
    "CALL",
    "SHAKE",
]

EVENTS = ["CALL", "MAIL", "TEAMS", "LOUD", "VERY_LOUD", "TILT", "SHAKE", "QUIET"]

STYLES = ["EVE_CINEMATIC"]

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
    "gyro_tilt_xy_pct",
    "gyro_tilt_z_pct",
    "gyro_tilt_cooldown_ms",
]

TUNE_DEFAULTS = {
    "drift_amp_px": "2",
    "saccade_amp_px": "5",
    "saccade_min_ms": "1400",
    "saccade_max_ms": "3800",
    "blink_interval_ms": "3600",
    "blink_duration_ms": "120",
    "double_blink_chance_pct": "20",
    "glow_pulse_amp": "6",
    "glow_pulse_period_ms": "2600",
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
        self.root.title("DeskRobo PC Agent")
        self.root.geometry("980x760")
        self.root.minsize(900, 700)

        self.event_queue: "queue.Queue[tuple]" = queue.Queue()
        self.controller = AgentController(self.event_queue)

        self.mode_var = tk.StringVar(value="basic")
        self.state_var = tk.StringVar(value="Nicht gestartet")
        self.detail_var = tk.StringVar(value="")

        self.emotion_var = tk.StringVar(value="IDLE")
        self.emotion_hold_var = tk.StringVar(value="3500")
        self.style_var = tk.StringVar(value="EVE_CINEMATIC")
        self.backlight_var = tk.IntVar(value=65)
        self.status_label_var = tk.BooleanVar(value=False)
        self.left_eye_var = tk.StringVar(value="IDLE")
        self.right_eye_var = tk.StringVar(value="IDLE")
        self.eye_hold_var = tk.StringVar(value="5000")
        self.raw_cmd_var = tk.StringVar(value="")

        self.tune_vars = {k: tk.StringVar(value=TUNE_DEFAULTS.get(k, "")) for k in TUNE_KEYS}

        self._build_ui()
        self._update_buttons()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(200, self._poll_events)

    def _build_ui(self) -> None:
        container = ttk.Frame(self.root, padding=12)
        container.pack(fill=tk.BOTH, expand=True)

        top = ttk.Frame(container)
        top.pack(fill=tk.X)

        ttk.Label(top, text="Modus:").pack(side=tk.LEFT)
        ttk.Combobox(
            top,
            textvariable=self.mode_var,
            values=("basic", "all", "teams", "mic"),
            width=12,
            state="readonly",
        ).pack(side=tk.LEFT, padx=(8, 16))

        self.start_btn = ttk.Button(top, text="Start", command=self._on_start)
        self.start_btn.pack(side=tk.LEFT)
        self.stop_btn = ttk.Button(top, text="Stop", command=self._on_stop)
        self.stop_btn.pack(side=tk.LEFT, padx=8)

        status = ttk.LabelFrame(container, text="Status", padding=10)
        status.pack(fill=tk.X, pady=(10, 10))
        ttk.Label(status, textvariable=self.state_var, font=("Segoe UI", 11, "bold")).pack(anchor="w")
        ttk.Label(status, textvariable=self.detail_var).pack(anchor="w", pady=(4, 0))

        notebook = ttk.Notebook(container)
        notebook.pack(fill=tk.BOTH, expand=True)

        control_tab = ttk.Frame(notebook, padding=10)
        tune_tab = ttk.Frame(notebook, padding=10)
        log_tab = ttk.Frame(notebook, padding=10)

        notebook.add(control_tab, text="Steuerung")
        notebook.add(tune_tab, text="Idle Tuning")
        notebook.add(log_tab, text="Log")

        self._build_control_tab(control_tab)
        self._build_tune_tab(tune_tab)
        self._build_log_tab(log_tab)

    def _build_control_tab(self, parent: ttk.Frame) -> None:
        style = ttk.LabelFrame(parent, text="Eye Style", padding=10)
        style.pack(fill=tk.X, pady=(0, 8))
        ttk.Combobox(style, textvariable=self.style_var, values=STYLES, state="readonly", width=18).pack(side=tk.LEFT)
        ttk.Button(style, text="Style anwenden", command=self._on_set_style).pack(side=tk.LEFT, padx=8)

        emo = ttk.LabelFrame(parent, text="Emotion", padding=10)
        emo.pack(fill=tk.X, pady=(0, 8))
        ttk.Combobox(emo, textvariable=self.emotion_var, values=EMOTIONS, state="readonly", width=14).pack(side=tk.LEFT)
        ttk.Label(emo, text="Hold (ms)").pack(side=tk.LEFT, padx=(10, 4))
        ttk.Entry(emo, textvariable=self.emotion_hold_var, width=8).pack(side=tk.LEFT)
        ttk.Button(emo, text="Emotion senden", command=self._on_set_emotion).pack(side=tk.LEFT, padx=8)

        eyes = ttk.LabelFrame(parent, text="EVE Eyes (Left/Right)", padding=10)
        eyes.pack(fill=tk.X, pady=(0, 8))
        ttk.Combobox(eyes, textvariable=self.left_eye_var, values=EMOTIONS, state="readonly", width=12).pack(side=tk.LEFT)
        ttk.Combobox(eyes, textvariable=self.right_eye_var, values=EMOTIONS, state="readonly", width=12).pack(side=tk.LEFT, padx=(6, 0))
        ttk.Label(eyes, text="Hold (ms)").pack(side=tk.LEFT, padx=(10, 4))
        ttk.Entry(eyes, textvariable=self.eye_hold_var, width=8).pack(side=tk.LEFT)
        ttk.Button(eyes, text="Eye Pair setzen", command=self._on_set_eyes).pack(side=tk.LEFT, padx=8)

        backlight = ttk.LabelFrame(parent, text="Backlight", padding=10)
        backlight.pack(fill=tk.X, pady=(0, 8))
        ttk.Scale(backlight, from_=0, to=100, variable=self.backlight_var, orient="horizontal").pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Label(backlight, textvariable=self.backlight_var, width=4).pack(side=tk.LEFT, padx=(8, 8))
        ttk.Button(backlight, text="Helligkeit setzen", command=self._on_set_backlight).pack(side=tk.LEFT)

        debug = ttk.LabelFrame(parent, text="Debug", padding=10)
        debug.pack(fill=tk.X, pady=(0, 8))
        ttk.Checkbutton(debug, text="Show bottom status label", variable=self.status_label_var).pack(side=tk.LEFT)
        ttk.Button(debug, text="Debug anwenden", command=self._on_set_status_label).pack(side=tk.LEFT, padx=8)

        events = ttk.LabelFrame(parent, text="Events Simulator", padding=10)
        events.pack(fill=tk.X, pady=(0, 8))
        for i, ev in enumerate(EVENTS):
            ttk.Button(events, text=ev, command=lambda n=ev: self._send_event(n)).grid(row=i // 4, column=i % 4, padx=4, pady=4, sticky="ew")
        for col in range(4):
            events.grid_columnconfigure(col, weight=1)

        misc = ttk.LabelFrame(parent, text="BLE Extras", padding=10)
        misc.pack(fill=tk.X)
        ttk.Button(misc, text="Zeit jetzt syncen", command=self._on_time_sync).pack(side=tk.LEFT)
        ttk.Entry(misc, textvariable=self.raw_cmd_var, width=36).pack(side=tk.LEFT, padx=(10, 6))
        ttk.Button(misc, text="Raw CMD senden", command=self._on_raw_cmd).pack(side=tk.LEFT)

    def _build_tune_tab(self, parent: ttk.Frame) -> None:
        frame = ttk.LabelFrame(parent, text="Idle Bewegung anpassen", padding=10)
        frame.pack(fill=tk.BOTH, expand=True)

        for i, key in enumerate(TUNE_KEYS):
            r = i // 2
            c = (i % 2) * 2
            ttk.Label(frame, text=key).grid(row=r, column=c, sticky="w", padx=(0, 8), pady=4)
            ttk.Entry(frame, textvariable=self.tune_vars[key], width=16).grid(row=r, column=c + 1, sticky="w", pady=4)

        btns = ttk.Frame(frame)
        btns.grid(row=7, column=0, columnspan=4, sticky="w", pady=(12, 0))
        ttk.Button(btns, text="Werte anwenden", command=self._on_apply_tune).pack(side=tk.LEFT)
        ttk.Button(btns, text="Als Standard speichern", command=self._on_save_tune).pack(side=tk.LEFT, padx=8)
        ttk.Button(btns, text="Gespeicherte Werte laden", command=self._on_load_tune).pack(side=tk.LEFT)

    def _build_log_tab(self, parent: ttk.Frame) -> None:
        self.log_text = tk.Text(parent, wrap="word", state="disabled")
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self._append_log("GUI bereit. Mit Start wird der Agent im Hintergrund gestartet.")

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
            "searching": "Suche DeskRobo",
            "connecting": "Verbinde",
            "connected": "Verbunden",
            "disconnected": "Nicht verbunden",
            "retry": "Neuversuch",
            "heartbeat_timeout": "Verbindung instabil",
            "not_found": "DeskRobo nicht gefunden",
            "stopping": "Stoppt",
            "stopped": "Gestoppt",
        }
        return mapping.get(state, state), message

    def _send(self, *cmd) -> bool:
        if not self.controller.is_running():
            self._append_log("Befehl nicht gesendet: Agent ist nicht gestartet.")
            return False
        ok = self.controller.send_command(*cmd)
        if not ok:
            self._append_log("Befehl konnte nicht in die Queue gelegt werden.")
        return ok

    def _on_start(self) -> None:
        mode = self.mode_var.get().strip() or "basic"
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

    def _on_set_style(self) -> None:
        if self._send("style", self.style_var.get()):
            self._append_log(f"Style gesendet: {self.style_var.get()}")

    def _on_set_emotion(self) -> None:
        try:
            hold = int(self.emotion_hold_var.get().strip())
        except ValueError:
            self._append_log("Hold muss eine ganze Zahl sein.")
            return
        emo = self.emotion_var.get()
        if self._send("emotion", emo, hold):
            self._append_log(f"Emotion gesendet: {emo} ({hold}ms)")

    def _on_set_eyes(self) -> None:
        try:
            hold = int(self.eye_hold_var.get().strip())
        except ValueError:
            self._append_log("Eye-Hold muss eine ganze Zahl sein.")
            return
        left = self.left_eye_var.get()
        right = self.right_eye_var.get()
        if self._send("eyes", left, right, hold):
            self._append_log(f"Eye Pair gesendet: {left}/{right} ({hold}ms)")

    def _on_set_backlight(self) -> None:
        value = int(self.backlight_var.get())
        if self._send("backlight", value):
            self._append_log(f"Backlight gesendet: {value}%")

    def _on_set_status_label(self) -> None:
        val = bool(self.status_label_var.get())
        if self._send("status_label", val):
            self._append_log(f"Status-Label gesetzt: {'ON' if val else 'OFF'}")

    def _send_event(self, name: str) -> None:
        if self._send("event", name):
            self._append_log(f"Event gesendet: {name}")

    def _on_time_sync(self) -> None:
        if self._send("time_sync"):
            self._append_log("Zeit-Sync gesendet.")

    def _on_raw_cmd(self) -> None:
        payload = self.raw_cmd_var.get().strip()
        if not payload:
            self._append_log("Raw CMD ist leer.")
            return
        if self._send("cmd", payload):
            self._append_log(f"Raw CMD gesendet: {payload}")

    def _on_apply_tune(self) -> None:
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
        self._append_log(f"Tuning gesendet: {sent} Werte")

    def _on_save_tune(self) -> None:
        if self._send("tune_save"):
            self._append_log("Tuning auf Device gespeichert.")

    def _on_load_tune(self) -> None:
        if self._send("tune_load"):
            self._append_log("Tuning vom Device geladen (aktive Werte wurden umgeschaltet).")

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
                self._append_log(f"{title}: {message}" if message else title)
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
