import asyncio
import queue
import threading
import tkinter as tk
from datetime import datetime
from tkinter import ttk

import pc_agent as agent_runtime


EMOTIONS = [
    "IDLE", "HAPPY", "SAD", "ANGRY", "WOW",
    "SLEEPY", "CONFUSED", "EXCITED", "DIZZY",
    "MAIL", "CALL", "SHAKE",
]

EMOTION_ICONS = {
    "IDLE": "😐", "HAPPY": "😄", "SAD": "😢", "ANGRY": "😠",
    "WOW": "😮", "SLEEPY": "😴", "CONFUSED": "😕", "EXCITED": "🤩",
    "DIZZY": "😵", "MAIL": "✉", "CALL": "📞", "SHAKE": "😱",
}

EVENTS = ["CALL", "MAIL", "TEAMS", "LOUD", "VERY_LOUD", "TILT", "SHAKE", "QUIET"]

EVENT_COLORS = {
    "CALL": "#0fdaff", "MAIL": "#58a6ff", "TEAMS": "#7c6ef7",
    "LOUD": "#ffa657", "VERY_LOUD": "#f85149", "TILT": "#3fb950",
    "SHAKE": "#ff6b6b", "QUIET": "#7d8590",
}

STYLES = ["EVE_CINEMATIC"]

TUNE_KEYS = [
    "drift_amp_px", "saccade_amp_px", "saccade_min_ms", "saccade_max_ms",
    "blink_interval_ms", "blink_duration_ms", "double_blink_chance_pct",
    "glow_pulse_amp", "glow_pulse_period_ms", "shake_amp_px", "shake_period_ms",
    "gyro_tilt_xy_pct", "gyro_tilt_z_pct", "gyro_tilt_cooldown_ms",
]

TUNE_LABELS = {
    "drift_amp_px":           "Blickdrift Stärke (px)",
    "saccade_amp_px":         "Sakkaden Sprungweite (px)",
    "saccade_min_ms":         "Sakkaden min Intervall (ms)",
    "saccade_max_ms":         "Sakkaden max Intervall (ms)",
    "blink_interval_ms":      "Blink Intervall (ms)",
    "blink_duration_ms":      "Blink Dauer (ms)",
    "double_blink_chance_pct":"Doppelblink Chance (%)",
    "glow_pulse_amp":         "Glow Puls Stärke",
    "glow_pulse_period_ms":   "Glow Puls Periode (ms)",
    "shake_amp_px":           "Shake Amplitude (px)",
    "shake_period_ms":        "Shake Geschwindigkeit (ms)",
    "gyro_tilt_xy_pct":       "Gyro Tilt XY Schwelle (%)",
    "gyro_tilt_z_pct":        "Gyro Tilt Z Schwelle (%)",
    "gyro_tilt_cooldown_ms":  "Gyro Tilt Cooldown (ms)",
}

TUNE_DEFAULTS = {
    "drift_amp_px": "2", "saccade_amp_px": "5",
    "saccade_min_ms": "1400", "saccade_max_ms": "3800",
    "blink_interval_ms": "3600", "blink_duration_ms": "120",
    "double_blink_chance_pct": "20", "glow_pulse_amp": "6",
    "glow_pulse_period_ms": "2600", "shake_amp_px": "24",
    "shake_period_ms": "700", "gyro_tilt_xy_pct": "62",
    "gyro_tilt_z_pct": "64", "gyro_tilt_cooldown_ms": "2200",
}

# ── Color Palette ────────────────────────────────────────────────────────────
C = {
    "bg":       "#0d1117",
    "surface":  "#161b22",
    "card":     "#1c2128",
    "border":   "#30363d",
    "accent":   "#0fdaff",
    "accent2":  "#0ab8d8",
    "text":     "#e6edf3",
    "dim":      "#7d8590",
    "muted":    "#484f58",
    "green":    "#3fb950",
    "yellow":   "#d29922",
    "red":      "#f85149",
    "btn":      "#21262d",
    "btn_hov":  "#30363d",
}

FONT      = ("Segoe UI", 9)
FONT_SM   = ("Segoe UI", 8)
FONT_MED  = ("Segoe UI", 10)
FONT_BOLD = ("Segoe UI", 10, "bold")
FONT_H1   = ("Segoe UI", 13, "bold")
FONT_H2   = ("Segoe UI", 9, "bold")
FONT_MONO = ("Consolas", 9)


# ── Helpers ──────────────────────────────────────────────────────────────────

def _section(parent, title: str, pady=(0, 12)) -> tk.Frame:
    """Card-section with accent left border and title label."""
    outer = tk.Frame(parent, bg=C["bg"])
    outer.pack(fill=tk.X, pady=pady)

    header = tk.Frame(outer, bg=C["bg"])
    header.pack(fill=tk.X, pady=(0, 6))
    tk.Frame(header, bg=C["accent"], width=3, height=14).pack(side=tk.LEFT, padx=(0, 8))
    tk.Label(header, text=title, bg=C["bg"], fg=C["dim"],
             font=FONT_H2).pack(side=tk.LEFT)

    card = tk.Frame(outer, bg=C["card"], padx=12, pady=10)
    card.pack(fill=tk.X)
    return card


def _label(parent, text, fg=None, font=None, **kw) -> tk.Label:
    return tk.Label(parent, text=text, bg=C["bg"], fg=fg or C["text"],
                    font=font or FONT, **kw)


def _card_label(parent, text, fg=None, font=None, **kw) -> tk.Label:
    return tk.Label(parent, text=text, bg=C["card"], fg=fg or C["text"],
                    font=font or FONT, **kw)


def _separator(parent) -> None:
    tk.Frame(parent, bg=C["border"], height=1).pack(fill=tk.X, pady=8)


class _HoverButton(tk.Button):
    """Flat button with hover highlight."""
    def __init__(self, master, accent=False, small=False, **kw):
        self._accent = accent
        bg     = C["accent"] if accent else C["btn"]
        fg     = C["bg"]     if accent else C["text"]
        abg    = C["accent2"]if accent else C["btn_hov"]
        relief = "flat"
        kw.setdefault("font", FONT_SM if small else FONT)
        kw.setdefault("cursor", "hand2")
        kw.setdefault("pady", 5)
        kw.setdefault("padx", 14)
        kw.setdefault("bd", 0)
        kw.setdefault("activebackground", abg)
        kw.setdefault("activeforeground", fg)
        super().__init__(master, bg=bg, fg=fg, relief=relief,
                         highlightthickness=0, **kw)
        self._normal_bg = bg
        self._hover_bg  = abg
        self.bind("<Enter>", lambda _: self.config(bg=self._hover_bg))
        self.bind("<Leave>", lambda _: self.config(bg=self._normal_bg))


class _DarkCombo(ttk.Combobox):
    pass


class _DarkEntry(tk.Entry):
    def __init__(self, master, **kw):
        kw.setdefault("bg", C["surface"])
        kw.setdefault("fg", C["text"])
        kw.setdefault("insertbackground", C["text"])
        kw.setdefault("relief", "flat")
        kw.setdefault("highlightthickness", 1)
        kw.setdefault("highlightbackground", C["border"])
        kw.setdefault("highlightcolor", C["accent"])
        kw.setdefault("font", FONT)
        kw.setdefault("bd", 4)
        super().__init__(master, **kw)


# ── Agent Controller (unchanged) ─────────────────────────────────────────────

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
                    mode, status_callback=status_callback,
                    stop_event=stop_event, control_queue=control_queue,
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


# ── Main App ─────────────────────────────────────────────────────────────────

class AgentApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("DeskRobo · PC Agent")
        self.root.geometry("1020x780")
        self.root.minsize(900, 680)
        self.root.configure(bg=C["bg"])

        self.event_queue: "queue.Queue[tuple]" = queue.Queue()
        self.controller = AgentController(self.event_queue)

        self.mode_var         = tk.StringVar(value="basic")
        self.state_var        = tk.StringVar(value="Nicht gestartet")
        self.detail_var       = tk.StringVar(value="")
        self.emotion_var      = tk.StringVar(value="IDLE")
        self.emotion_hold_var = tk.StringVar(value="3500")
        self.style_var        = tk.StringVar(value="EVE_CINEMATIC")
        self.backlight_var    = tk.IntVar(value=65)
        self.status_label_var = tk.BooleanVar(value=False)
        self.left_eye_var     = tk.StringVar(value="IDLE")
        self.right_eye_var    = tk.StringVar(value="IDLE")
        self.eye_hold_var     = tk.StringVar(value="5000")
        self.raw_cmd_var      = tk.StringVar(value="")
        self.tune_vars        = {k: tk.StringVar(value=TUNE_DEFAULTS.get(k, "")) for k in TUNE_KEYS}

        self._setup_ttk_style()
        self._build_ui()
        self._update_buttons()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(200, self._poll_events)

    # ── TTK Style ────────────────────────────────────────────────────────────

    def _setup_ttk_style(self) -> None:
        s = ttk.Style()
        s.theme_use("clam")

        s.configure("TFrame",        background=C["bg"])
        s.configure("Card.TFrame",   background=C["card"])
        s.configure("Surface.TFrame",background=C["surface"])

        s.configure("TLabel",        background=C["bg"],    foreground=C["text"],  font=FONT)
        s.configure("Dim.TLabel",    background=C["bg"],    foreground=C["dim"],   font=FONT_SM)
        s.configure("Card.TLabel",   background=C["card"],  foreground=C["text"],  font=FONT)
        s.configure("CardDim.TLabel",background=C["card"],  foreground=C["dim"],   font=FONT_SM)
        s.configure("Mono.TLabel",   background=C["card"],  foreground=C["accent"],font=FONT_MONO)

        # Notebook tab bar
        s.configure("TNotebook",
            background=C["bg"], bordercolor=C["border"], tabmargins=[0, 0, 0, 0])
        s.configure("TNotebook.Tab",
            background=C["surface"], foreground=C["dim"],
            font=FONT, padding=[18, 9], bordercolor=C["border"])
        s.map("TNotebook.Tab",
            background=[("selected", C["card"]), ("active", C["btn"])],
            foreground=[("selected", C["text"]), ("active", C["text"])])

        # Combobox
        s.configure("TCombobox",
            fieldbackground=C["surface"], background=C["card"],
            foreground=C["text"], selectbackground=C["surface"],
            selectforeground=C["text"], bordercolor=C["border"],
            arrowcolor=C["dim"], insertcolor=C["text"])
        s.map("TCombobox",
            fieldbackground=[("readonly", C["surface"])],
            foreground=[("readonly", C["text"])])

        # Scale
        s.configure("TScale",
            background=C["bg"], troughcolor=C["surface"],
            darkcolor=C["accent"], lightcolor=C["accent"],
            sliderlength=18, sliderrelief="flat")
        s.map("TScale", background=[("active", C["bg"])])

        # Checkbutton
        s.configure("TCheckbutton",
            background=C["bg"], foreground=C["text"],
            font=FONT, indicatorcolor=C["surface"],
            indicatordiameter=14)
        s.map("TCheckbutton",
            indicatorcolor=[("selected", C["accent"]), ("active", C["btn_hov"])],
            background=[("active", C["bg"])])

    # ── UI Build ─────────────────────────────────────────────────────────────

    def _build_ui(self) -> None:
        # ── Titlebar ──────────────────────────────────────────────────────
        titlebar = tk.Frame(self.root, bg=C["surface"], height=52)
        titlebar.pack(fill=tk.X)
        titlebar.pack_propagate(False)

        # Logo dot
        dot_canvas = tk.Canvas(titlebar, width=10, height=10,
                               bg=C["surface"], highlightthickness=0)
        dot_canvas.pack(side=tk.LEFT, padx=(18, 0), pady=21)
        dot_canvas.create_oval(0, 0, 10, 10, fill=C["accent"], outline="")

        tk.Label(titlebar, text="DeskRobo", bg=C["surface"],
                 fg=C["text"], font=FONT_H1).pack(side=tk.LEFT, padx=(8, 0))
        tk.Label(titlebar, text="PC Agent", bg=C["surface"],
                 fg=C["dim"], font=FONT_MED).pack(side=tk.LEFT, padx=(8, 0), pady=2)

        # Version badge
        tk.Label(titlebar, text="v2  ", bg=C["surface"],
                 fg=C["muted"], font=FONT_SM).pack(side=tk.RIGHT, padx=8)

        tk.Frame(self.root, bg=C["border"], height=1).pack(fill=tk.X)

        # ── Main layout ───────────────────────────────────────────────────
        main = tk.Frame(self.root, bg=C["bg"])
        main.pack(fill=tk.BOTH, expand=True)

        # Left sidebar
        sidebar = tk.Frame(main, bg=C["surface"], width=220)
        sidebar.pack(side=tk.LEFT, fill=tk.Y)
        sidebar.pack_propagate(False)
        tk.Frame(main, bg=C["border"], width=1).pack(side=tk.LEFT, fill=tk.Y)

        # Content
        content = tk.Frame(main, bg=C["bg"])
        content.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self._build_sidebar(sidebar)
        self._build_content(content)

    def _build_sidebar(self, parent: tk.Frame) -> None:
        pad = tk.Frame(parent, bg=C["surface"], padx=16, pady=20)
        pad.pack(fill=tk.BOTH, expand=True)

        # Status card
        status_card = tk.Frame(pad, bg=C["card"], padx=14, pady=14)
        status_card.pack(fill=tk.X)

        # Dot + state row
        dot_row = tk.Frame(status_card, bg=C["card"])
        dot_row.pack(fill=tk.X, pady=(0, 6))
        self._status_dot = tk.Canvas(dot_row, width=10, height=10,
                                     bg=C["card"], highlightthickness=0)
        self._status_dot.pack(side=tk.LEFT, pady=3)
        self._status_dot.create_oval(1, 1, 9, 9, fill=C["muted"], outline="", tags="dot")
        tk.Label(dot_row, text="  Status", bg=C["card"],
                 fg=C["dim"], font=FONT_SM).pack(side=tk.LEFT)

        self._state_lbl = tk.Label(status_card, textvariable=self.state_var,
                                   bg=C["card"], fg=C["text"], font=FONT_BOLD,
                                   anchor="w")
        self._state_lbl.pack(fill=tk.X)
        tk.Label(status_card, textvariable=self.detail_var,
                 bg=C["card"], fg=C["dim"], font=FONT_SM,
                 anchor="w", wraplength=170, justify="left").pack(fill=tk.X, pady=(4, 0))

        # Separator
        tk.Frame(pad, bg=C["border"], height=1).pack(fill=tk.X, pady=16)

        # Mode selector
        tk.Label(pad, text="MODUS", bg=C["surface"],
                 fg=C["muted"], font=("Segoe UI", 8, "bold")).pack(anchor="w", pady=(0, 6))

        mode_frame = tk.Frame(pad, bg=C["surface"])
        mode_frame.pack(fill=tk.X)
        combo = _DarkCombo(mode_frame, textvariable=self.mode_var,
                           values=("basic", "all", "teams", "mic"),
                           state="readonly", width=10)
        combo.pack(side=tk.LEFT, fill=tk.X, expand=True)

        tk.Frame(pad, bg=C["surface"], height=10).pack()

        # Start / Stop
        self.start_btn = _HoverButton(pad, accent=True, text="▶  Start",
                                      command=self._on_start, font=FONT_BOLD,
                                      pady=8)
        self.start_btn.pack(fill=tk.X, pady=(0, 6))

        self.stop_btn = _HoverButton(pad, text="■  Stop",
                                     command=self._on_stop, font=FONT)
        self.stop_btn.pack(fill=tk.X)

        tk.Frame(pad, bg=C["border"], height=1).pack(fill=tk.X, pady=16)

        # Backlight slider
        tk.Label(pad, text="BACKLIGHT", bg=C["surface"],
                 fg=C["muted"], font=("Segoe UI", 8, "bold")).pack(anchor="w", pady=(0, 4))

        sl_row = tk.Frame(pad, bg=C["surface"])
        sl_row.pack(fill=tk.X)
        ttk.Scale(sl_row, from_=0, to=100, variable=self.backlight_var,
                  orient="horizontal").pack(side=tk.LEFT, fill=tk.X, expand=True)
        tk.Label(sl_row, textvariable=self.backlight_var, width=3,
                 bg=C["surface"], fg=C["accent"], font=FONT_SM).pack(side=tk.LEFT, padx=(6, 0))

        _HoverButton(pad, text="Setzen", command=self._on_set_backlight,
                     small=True).pack(fill=tk.X, pady=(6, 0))

        tk.Frame(pad, bg=C["border"], height=1).pack(fill=tk.X, pady=16)

        # Debug toggle
        tk.Label(pad, text="DEBUG", bg=C["surface"],
                 fg=C["muted"], font=("Segoe UI", 8, "bold")).pack(anchor="w", pady=(0, 6))

        chk_frame = tk.Frame(pad, bg=C["surface"])
        chk_frame.pack(fill=tk.X)
        ttk.Checkbutton(chk_frame, text="Status Label",
                        variable=self.status_label_var,
                        command=self._on_set_status_label).pack(anchor="w")

        # Bottom: time sync
        tk.Frame(pad, bg=C["surface"]).pack(fill=tk.BOTH, expand=True)
        tk.Frame(pad, bg=C["border"], height=1).pack(fill=tk.X, pady=12)
        _HoverButton(pad, text="🕐  Zeit synchronisieren",
                     command=self._on_time_sync, small=True).pack(fill=tk.X)

    def _build_content(self, parent: tk.Frame) -> None:
        notebook = ttk.Notebook(parent)
        notebook.pack(fill=tk.BOTH, expand=True, padx=0, pady=0)

        control_tab = tk.Frame(notebook, bg=C["bg"])
        tune_tab    = tk.Frame(notebook, bg=C["bg"])
        log_tab     = tk.Frame(notebook, bg=C["bg"])

        notebook.add(control_tab, text="  Steuerung  ")
        notebook.add(tune_tab,    text="  Idle Tuning  ")
        notebook.add(log_tab,     text="  Log  ")

        self._build_control_tab(control_tab)
        self._build_tune_tab(tune_tab)
        self._build_log_tab(log_tab)

    # ── Control Tab ──────────────────────────────────────────────────────────

    def _build_control_tab(self, parent: tk.Frame) -> None:
        scroll_canvas = tk.Canvas(parent, bg=C["bg"], highlightthickness=0)
        scrollbar = tk.Scrollbar(parent, orient="vertical",
                                 command=scroll_canvas.yview)
        scroll_canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        scroll_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        inner = tk.Frame(scroll_canvas, bg=C["bg"])
        win_id = scroll_canvas.create_window((0, 0), window=inner, anchor="nw")

        def _on_configure(e):
            scroll_canvas.configure(scrollregion=scroll_canvas.bbox("all"))
        def _on_canvas_resize(e):
            scroll_canvas.itemconfig(win_id, width=e.width)
        inner.bind("<Configure>", _on_configure)
        scroll_canvas.bind("<Configure>", _on_canvas_resize)
        inner.bind("<MouseWheel>", lambda e: scroll_canvas.yview_scroll(-1*(e.delta//120), "units"))

        pad = tk.Frame(inner, bg=C["bg"], padx=24, pady=20)
        pad.pack(fill=tk.X)

        # ── Emotion grid ─────────────────────────────────────────────────
        emo_card = _section(pad, "EMOTION")
        emo_card.configure(bg=C["card"])

        # Grid of emotion buttons
        grid = tk.Frame(emo_card, bg=C["card"])
        grid.pack(fill=tk.X, pady=(0, 10))
        self._emo_btns = {}
        cols = 4
        for i, em in enumerate(EMOTIONS):
            icon = EMOTION_ICONS.get(em, "")
            btn = tk.Button(
                grid,
                text=f"{icon}\n{em}",
                bg=C["btn"], fg=C["dim"],
                font=("Segoe UI", 8), relief="flat",
                bd=0, padx=8, pady=8, cursor="hand2",
                highlightthickness=1, highlightbackground=C["border"],
                command=lambda e=em: self._select_emotion(e),
                activebackground=C["btn_hov"], activeforeground=C["text"],
            )
            btn.grid(row=i // cols, column=i % cols, padx=4, pady=4, sticky="nsew")
            self._emo_btns[em] = btn
        for c in range(cols):
            grid.grid_columnconfigure(c, weight=1)
        self._select_emotion("IDLE", send=False)

        # Hold + send row
        hold_row = tk.Frame(emo_card, bg=C["card"])
        hold_row.pack(fill=tk.X)
        tk.Label(hold_row, text="Hold (ms)", bg=C["card"],
                 fg=C["dim"], font=FONT_SM).pack(side=tk.LEFT)
        _DarkEntry(hold_row, textvariable=self.emotion_hold_var, width=8).pack(side=tk.LEFT, padx=(8, 12))
        _HoverButton(hold_row, accent=True, text="Emotion senden",
                     command=self._on_set_emotion).pack(side=tk.LEFT)

        # ── Eye Pair ─────────────────────────────────────────────────────
        eye_card = _section(pad, "EVE EYE PAIR")
        eye_card.configure(bg=C["card"])

        eye_row = tk.Frame(eye_card, bg=C["card"])
        eye_row.pack(fill=tk.X)

        tk.Label(eye_row, text="Links", bg=C["card"], fg=C["dim"], font=FONT_SM).grid(row=0, column=0, sticky="w", padx=(0,4))
        tk.Label(eye_row, text="Rechts", bg=C["card"], fg=C["dim"], font=FONT_SM).grid(row=0, column=1, sticky="w", padx=(8,4))
        tk.Label(eye_row, text="Hold (ms)", bg=C["card"], fg=C["dim"], font=FONT_SM).grid(row=0, column=2, sticky="w", padx=(16,4))

        _DarkCombo(eye_row, textvariable=self.left_eye_var,
                   values=EMOTIONS, state="readonly", width=12).grid(row=1, column=0, sticky="ew", padx=(0,4), pady=(4,0))
        _DarkCombo(eye_row, textvariable=self.right_eye_var,
                   values=EMOTIONS, state="readonly", width=12).grid(row=1, column=1, sticky="ew", padx=(8,4), pady=(4,0))
        _DarkEntry(eye_row, textvariable=self.eye_hold_var,
                   width=8).grid(row=1, column=2, sticky="w", padx=(16,12), pady=(4,0))
        _HoverButton(eye_row, accent=True, text="Setzen",
                     command=self._on_set_eyes).grid(row=1, column=3, padx=(8,0), pady=(4,0))

        # ── Eye Style ─────────────────────────────────────────────────────
        style_card = _section(pad, "EYE STYLE")
        style_card.configure(bg=C["card"])
        style_row = tk.Frame(style_card, bg=C["card"])
        style_row.pack(fill=tk.X)
        _DarkCombo(style_row, textvariable=self.style_var,
                   values=STYLES, state="readonly", width=20).pack(side=tk.LEFT)
        _HoverButton(style_row, accent=True, text="Anwenden",
                     command=self._on_set_style).pack(side=tk.LEFT, padx=(12, 0))

        # ── Events ────────────────────────────────────────────────────────
        ev_card = _section(pad, "EVENTS SIMULATOR")
        ev_card.configure(bg=C["card"])
        ev_grid = tk.Frame(ev_card, bg=C["card"])
        ev_grid.pack(fill=tk.X)
        cols = 4
        for i, ev in enumerate(EVENTS):
            color = EVENT_COLORS.get(ev, C["dim"])
            btn = tk.Button(
                ev_grid, text=ev,
                bg=C["btn"], fg=color,
                font=("Segoe UI", 8, "bold"), relief="flat",
                bd=0, padx=10, pady=7, cursor="hand2",
                highlightthickness=1, highlightbackground=color + "55",
                command=lambda n=ev: self._send_event(n),
                activebackground=C["btn_hov"], activeforeground=color,
            )
            btn.grid(row=i // cols, column=i % cols, padx=4, pady=4, sticky="nsew")
        for c in range(cols):
            ev_grid.grid_columnconfigure(c, weight=1)

        # ── Raw CMD ───────────────────────────────────────────────────────
        raw_card = _section(pad, "BLE RAW COMMAND", pady=(0, 0))
        raw_card.configure(bg=C["card"])
        raw_row = tk.Frame(raw_card, bg=C["card"])
        raw_row.pack(fill=tk.X)
        _DarkEntry(raw_row, textvariable=self.raw_cmd_var,
                   width=36).pack(side=tk.LEFT, fill=tk.X, expand=True)
        _HoverButton(raw_row, text="Senden",
                     command=self._on_raw_cmd).pack(side=tk.LEFT, padx=(10, 0))

    def _select_emotion(self, em: str, send: bool = True) -> None:
        self.emotion_var.set(em)
        for name, btn in self._emo_btns.items():
            if name == em:
                btn.config(bg=C["card"], fg=C["accent"],
                           highlightbackground=C["accent"])
            else:
                btn.config(bg=C["btn"], fg=C["dim"],
                           highlightbackground=C["border"])

    # ── Tune Tab ─────────────────────────────────────────────────────────────

    def _build_tune_tab(self, parent: tk.Frame) -> None:
        pad = tk.Frame(parent, bg=C["bg"], padx=24, pady=20)
        pad.pack(fill=tk.BOTH, expand=True)

        # Preset buttons
        preset_sec = _section(pad, "PRESETS", pady=(0, 16))
        preset_sec.configure(bg=C["card"])
        preset_row = tk.Frame(preset_sec, bg=C["card"])
        preset_row.pack(fill=tk.X)

        presets_info = [
            ("🌙  Ruhig",    "calm",     C["dim"]),
            ("⚖  Standard", "balanced", C["accent"]),
            ("⚡  Lebhaft",  "lively",   C["yellow"]),
        ]
        for label, name, color in presets_info:
            btn = tk.Button(
                preset_row, text=label,
                bg=C["btn"], fg=color, font=FONT,
                relief="flat", bd=0, padx=16, pady=7, cursor="hand2",
                highlightthickness=1, highlightbackground=color + "44",
                command=lambda n=name: self._apply_tune_preset(n),
                activebackground=C["btn_hov"], activeforeground=color,
            )
            btn.pack(side=tk.LEFT, padx=(0, 8))

        # Parameter grid
        tune_sec = _section(pad, "PARAMETER", pady=(0, 16))
        tune_sec.configure(bg=C["card"])

        for i, key in enumerate(TUNE_KEYS):
            r = i // 2
            c = (i % 2) * 3
            label_text = TUNE_LABELS.get(key, key)
            tk.Label(tune_sec, text=label_text, bg=C["card"],
                     fg=C["dim"], font=FONT_SM).grid(
                row=r, column=c, sticky="w", padx=(0, 8), pady=5)
            _DarkEntry(tune_sec, textvariable=self.tune_vars[key],
                       width=10).grid(row=r, column=c + 1, sticky="w", pady=5)
            if c == 0:
                tk.Frame(tune_sec, bg=C["border"], width=1).grid(
                    row=r, column=2, sticky="ns", padx=16, pady=2)

        # Actions
        action_sec = _section(pad, "AKTIONEN", pady=(0, 0))
        action_sec.configure(bg=C["card"])
        act_row = tk.Frame(action_sec, bg=C["card"])
        act_row.pack(fill=tk.X)
        _HoverButton(act_row, accent=True, text="Anwenden",
                     command=self._on_apply_tune).pack(side=tk.LEFT)
        _HoverButton(act_row, text="Auf Device speichern",
                     command=self._on_save_tune).pack(side=tk.LEFT, padx=(8, 0))
        _HoverButton(act_row, text="Von Device laden",
                     command=self._on_load_tune).pack(side=tk.LEFT, padx=(8, 0))

    # ── Log Tab ───────────────────────────────────────────────────────────────

    def _build_log_tab(self, parent: tk.Frame) -> None:
        toolbar = tk.Frame(parent, bg=C["surface"], padx=12, pady=8)
        toolbar.pack(fill=tk.X)
        tk.Label(toolbar, text="Systemlog", bg=C["surface"],
                 fg=C["dim"], font=FONT_SM).pack(side=tk.LEFT)
        _HoverButton(toolbar, text="Löschen", small=True,
                     command=self._clear_log).pack(side=tk.RIGHT)
        tk.Frame(parent, bg=C["border"], height=1).pack(fill=tk.X)

        self.log_text = tk.Text(
            parent, wrap="word", state="disabled",
            bg=C["bg"], fg=C["text"], font=FONT_MONO,
            insertbackground=C["text"], selectbackground=C["btn"],
            borderwidth=0, highlightthickness=0, padx=16, pady=12,
            relief="flat",
        )
        self.log_text.tag_config("ts",  foreground=C["muted"])
        self.log_text.tag_config("ok",  foreground=C["green"])
        self.log_text.tag_config("err", foreground=C["red"])
        self.log_text.tag_config("acc", foreground=C["accent"])

        scrollbar = tk.Scrollbar(parent, command=self.log_text.yview,
                                 bg=C["surface"], troughcolor=C["bg"],
                                 relief="flat", bd=0)
        self.log_text.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_text.pack(fill=tk.BOTH, expand=True)

        self._append_log("GUI bereit. Mit Start wird der Agent gestartet.", tag="acc")

    def _clear_log(self) -> None:
        self.log_text.configure(state="normal")
        self.log_text.delete("1.0", tk.END)
        self.log_text.configure(state="disabled")

    # ── Logging ──────────────────────────────────────────────────────────────

    def _append_log(self, line: str, tag: str = "") -> None:
        ts = datetime.now().strftime("%H:%M:%S")
        self.log_text.configure(state="normal")
        self.log_text.insert(tk.END, f"[{ts}]  ", "ts")
        self.log_text.insert(tk.END, f"{line}\n", tag or "")
        self.log_text.see(tk.END)
        self.log_text.configure(state="disabled")

    # ── Status ───────────────────────────────────────────────────────────────

    def _set_state(self, state: str, detail: str = "") -> None:
        self.state_var.set(state)
        self.detail_var.set(detail)

        color_map = {
            "Verbunden":        C["green"],
            "Startet":          C["yellow"],
            "Suche DeskRobo":   C["yellow"],
            "Verbinde":         C["yellow"],
            "Verbindung instabil": C["yellow"],
            "Stoppt":           C["dim"],
            "Gestoppt":         C["muted"],
            "Nicht gestartet":  C["muted"],
            "Fehler":           C["red"],
        }
        dot_color = color_map.get(state, C["muted"])
        self._status_dot.itemconfig("dot", fill=dot_color)
        self._state_lbl.config(fg=dot_color)

    def _friendly_status(self, state: str, message: str) -> tuple[str, str]:
        mapping = {
            "starting":          "Startet",
            "searching":         "Suche DeskRobo",
            "connecting":        "Verbinde",
            "connected":         "Verbunden",
            "disconnected":      "Nicht verbunden",
            "retry":             "Neuversuch",
            "heartbeat_timeout": "Verbindung instabil",
            "not_found":         "DeskRobo nicht gefunden",
            "stopping":          "Stoppt",
            "stopped":           "Gestoppt",
        }
        return mapping.get(state, state), message

    # ── Commands ─────────────────────────────────────────────────────────────

    def _send(self, *cmd) -> bool:
        if not self.controller.is_running():
            self._append_log("Befehl nicht gesendet: Agent ist nicht gestartet.", tag="err")
            return False
        ok = self.controller.send_command(*cmd)
        if not ok:
            self._append_log("Befehl konnte nicht in die Queue gelegt werden.", tag="err")
        return ok

    def _on_start(self) -> None:
        mode = self.mode_var.get().strip() or "basic"
        if self.controller.start(mode):
            self._set_state("Startet", f"Modus: {mode}")
            self._append_log(f"Agent gestartet (Modus: {mode}).", tag="ok")
        else:
            self._append_log("Agent läuft bereits.", tag="err")
        self._update_buttons()

    def _on_stop(self) -> None:
        self.controller.stop()
        self._set_state("Stoppt", "Beende Hintergrundprozess…")
        self._append_log("Stop angefordert.")
        self._update_buttons()

    def _on_set_style(self) -> None:
        if self._send("style", self.style_var.get()):
            self._append_log(f"Style gesendet: {self.style_var.get()}", tag="ok")

    def _on_set_emotion(self) -> None:
        try:
            hold = int(self.emotion_hold_var.get().strip())
        except ValueError:
            self._append_log("Hold muss eine ganze Zahl sein.", tag="err")
            return
        emo = self.emotion_var.get()
        if self._send("emotion", emo, hold):
            self._append_log(f"Emotion gesendet: {emo} ({hold} ms)", tag="ok")

    def _on_set_eyes(self) -> None:
        try:
            hold = int(self.eye_hold_var.get().strip())
        except ValueError:
            self._append_log("Eye-Hold muss eine ganze Zahl sein.", tag="err")
            return
        left, right = self.left_eye_var.get(), self.right_eye_var.get()
        if self._send("eyes", left, right, hold):
            self._append_log(f"Eye Pair gesendet: {left} / {right} ({hold} ms)", tag="ok")

    def _on_set_backlight(self) -> None:
        value = int(self.backlight_var.get())
        if self._send("backlight", value):
            self._append_log(f"Backlight gesendet: {value}%", tag="ok")

    def _on_set_status_label(self) -> None:
        val = bool(self.status_label_var.get())
        if self._send("status_label", val):
            self._append_log(f"Status-Label: {'ON' if val else 'OFF'}", tag="ok")

    def _send_event(self, name: str) -> None:
        if self._send("event", name):
            self._append_log(f"Event gesendet: {name}", tag="ok")

    def _on_time_sync(self) -> None:
        if self._send("time_sync"):
            self._append_log("Zeit-Sync gesendet.", tag="ok")

    def _on_raw_cmd(self) -> None:
        payload = self.raw_cmd_var.get().strip()
        if not payload:
            self._append_log("Raw CMD ist leer.", tag="err")
            return
        if self._send("cmd", payload):
            self._append_log(f"Raw CMD: {payload}", tag="ok")

    def _apply_tune_preset(self, name: str) -> None:
        presets = {
            "calm": {
                "drift_amp_px": 1, "saccade_amp_px": 2, "saccade_min_ms": 2200,
                "saccade_max_ms": 5200, "blink_interval_ms": 5200, "blink_duration_ms": 100,
                "double_blink_chance_pct": 8, "glow_pulse_amp": 4, "glow_pulse_period_ms": 3200,
                "shake_amp_px": 14, "shake_period_ms": 900, "gyro_tilt_xy_pct": 72,
                "gyro_tilt_z_pct": 74, "gyro_tilt_cooldown_ms": 2800,
            },
            "balanced": {
                "drift_amp_px": 2, "saccade_amp_px": 5, "saccade_min_ms": 1400,
                "saccade_max_ms": 3800, "blink_interval_ms": 3600, "blink_duration_ms": 120,
                "double_blink_chance_pct": 20, "glow_pulse_amp": 6, "glow_pulse_period_ms": 2600,
                "shake_amp_px": 24, "shake_period_ms": 700, "gyro_tilt_xy_pct": 62,
                "gyro_tilt_z_pct": 64, "gyro_tilt_cooldown_ms": 2200,
            },
            "lively": {
                "drift_amp_px": 4, "saccade_amp_px": 8, "saccade_min_ms": 900,
                "saccade_max_ms": 2500, "blink_interval_ms": 2800, "blink_duration_ms": 130,
                "double_blink_chance_pct": 30, "glow_pulse_amp": 10, "glow_pulse_period_ms": 2000,
                "shake_amp_px": 32, "shake_period_ms": 520, "gyro_tilt_xy_pct": 56,
                "gyro_tilt_z_pct": 58, "gyro_tilt_cooldown_ms": 1600,
            },
        }
        p = presets.get(name)
        if not p:
            return
        for key, value in p.items():
            if key in self.tune_vars:
                self.tune_vars[key].set(str(value))
        self._on_apply_tune()
        self._append_log(f"Preset angewendet: {name}", tag="ok")

    def _on_apply_tune(self) -> None:
        sent = 0
        for key in TUNE_KEYS:
            raw = self.tune_vars[key].get().strip()
            if not raw:
                continue
            try:
                val = int(raw)
            except ValueError:
                self._append_log(f"Ungültiger Wert für {key}: {raw}", tag="err")
                continue
            if self._send("tune", key, val):
                sent += 1
        self._append_log(f"Tuning gesendet: {sent} Werte", tag="ok")

    def _on_save_tune(self) -> None:
        if self._send("tune_save"):
            self._append_log("Tuning auf Device gespeichert.", tag="ok")

    def _on_load_tune(self) -> None:
        if self._send("tune_load"):
            self._append_log("Tuning vom Device geladen.", tag="ok")

    # ── Buttons / Poll ───────────────────────────────────────────────────────

    def _update_buttons(self) -> None:
        running = self.controller.is_running()
        self.start_btn.config(state="disabled" if running else "normal")
        self.stop_btn.config(state="normal" if running else "disabled")

    def _poll_events(self) -> None:
        while True:
            try:
                kind, state, message = self.event_queue.get_nowait()
            except queue.Empty:
                break

            if kind == "status":
                title, detail = self._friendly_status(state, message)
                self._set_state(title, detail)
                tag = "ok" if state == "connected" else ("err" if state in ("error", "not_found") else "")
                self._append_log(f"{title}: {message}" if message else title, tag=tag)
            elif kind == "error":
                self._set_state("Fehler", message)
                self._append_log(f"Fehler: {message}", tag="err")
            elif kind == "stopped":
                self._set_state("Gestoppt", "")
                self._append_log("Agent wurde beendet.")

        self._update_buttons()
        self.root.after(200, self._poll_events)

    def _on_close(self) -> None:
        if self.controller.is_running():
            self.controller.stop()
        self.root.after(150, self.root.destroy)


# ── Entry Point ──────────────────────────────────────────────────────────────

def main() -> None:
    root = tk.Tk()
    root.configure(bg=C["bg"])
    # Remove ugly default window icon border on Windows
    try:
        root.iconbitmap(default="")
    except Exception:
        pass
    AgentApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
