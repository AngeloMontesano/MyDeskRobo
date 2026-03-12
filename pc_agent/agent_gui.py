import asyncio
import queue
import threading
import tkinter as tk
from datetime import datetime
from tkinter import ttk

import pc_agent as agent_runtime


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

    def send_tune(self, key: str, value: int) -> bool:
        with self._lock:
            loop = self._loop
            control_queue = self._control_queue
        if loop is None or control_queue is None:
            return False
        try:
            loop.call_soon_threadsafe(control_queue.put_nowait, ("tune", key, int(value)))
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
        self.root.geometry("620x560")
        self.root.minsize(580, 500)

        self.event_queue: "queue.Queue[tuple]" = queue.Queue()
        self.controller = AgentController(self.event_queue)

        self.mode_var = tk.StringVar(value="basic")
        self.state_var = tk.StringVar(value="Nicht gestartet")
        self.detail_var = tk.StringVar(value="")
        self.gyro_xy_var = tk.StringVar(value="62")
        self.gyro_z_var = tk.StringVar(value="64")
        self.gyro_cd_var = tk.StringVar(value="2200")

        self._build_ui()
        self._update_buttons()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(200, self._poll_events)

    def _build_ui(self) -> None:
        container = ttk.Frame(self.root, padding=14)
        container.pack(fill=tk.BOTH, expand=True)

        top = ttk.Frame(container)
        top.pack(fill=tk.X)

        ttk.Label(top, text="Modus:").pack(side=tk.LEFT)
        mode_combo = ttk.Combobox(
            top,
            textvariable=self.mode_var,
            values=("basic", "all", "teams", "mic"),
            width=12,
            state="readonly",
        )
        mode_combo.pack(side=tk.LEFT, padx=(8, 16))

        self.start_btn = ttk.Button(top, text="Start", command=self._on_start)
        self.start_btn.pack(side=tk.LEFT)

        self.stop_btn = ttk.Button(top, text="Stop", command=self._on_stop)
        self.stop_btn.pack(side=tk.LEFT, padx=8)

        status = ttk.LabelFrame(container, text="Status", padding=12)
        status.pack(fill=tk.X, pady=(12, 8))

        ttk.Label(status, textvariable=self.state_var, font=("Segoe UI", 11, "bold")).pack(anchor="w")
        ttk.Label(status, textvariable=self.detail_var).pack(anchor="w", pady=(4, 0))

        tune = ttk.LabelFrame(container, text="Gyro / Confused Tuning", padding=12)
        tune.pack(fill=tk.X, pady=(8, 8))

        grid = ttk.Frame(tune)
        grid.pack(fill=tk.X)

        ttk.Label(grid, text="Tilt XY Schwelle (%)").grid(row=0, column=0, sticky="w")
        ttk.Entry(grid, textvariable=self.gyro_xy_var, width=10).grid(row=0, column=1, padx=(8, 16), sticky="w")

        ttk.Label(grid, text="Tilt Z Schwelle (%)").grid(row=0, column=2, sticky="w")
        ttk.Entry(grid, textvariable=self.gyro_z_var, width=10).grid(row=0, column=3, padx=(8, 0), sticky="w")

        ttk.Label(grid, text="Cooldown (ms)").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(grid, textvariable=self.gyro_cd_var, width=10).grid(row=1, column=1, padx=(8, 16), sticky="w", pady=(8, 0))

        self.apply_tune_btn = ttk.Button(tune, text="Gyro-Werte anwenden", command=self._on_apply_gyro_tuning)
        self.apply_tune_btn.pack(anchor="w", pady=(10, 0))

        ttk.Label(
            tune,
            text="Hinweis: Funktioniert nur, wenn der Agent bereits gestartet und verbunden ist.",
        ).pack(anchor="w", pady=(6, 0))

        log_frame = ttk.LabelFrame(container, text="Ereignisse", padding=8)
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(8, 0))

        self.log_text = tk.Text(log_frame, height=12, wrap="word", state="disabled")
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

    def _on_apply_gyro_tuning(self) -> None:
        if not self.controller.is_running():
            self._append_log("Gyro-Tuning nicht gesendet: Agent ist nicht gestartet.")
            return

        try:
            xy = int(self.gyro_xy_var.get().strip())
            z = int(self.gyro_z_var.get().strip())
            cooldown = int(self.gyro_cd_var.get().strip())
        except ValueError:
            self._append_log("Ungueltige Gyro-Werte: Bitte nur ganze Zahlen eingeben.")
            return

        payloads = [
            ("gyro_tilt_xy_pct", xy),
            ("gyro_tilt_z_pct", z),
            ("gyro_tilt_cooldown_ms", cooldown),
        ]

        sent = 0
        for key, value in payloads:
            if self.controller.send_tune(key, value):
                sent += 1

        if sent == len(payloads):
            self._append_log(f"Gyro-Tuning gesendet: XY={xy}, Z={z}, Cooldown={cooldown}ms")
        else:
            self._append_log("Gyro-Tuning nur teilweise gesendet. Bitte Verbindung pruefen.")

    def _update_buttons(self) -> None:
        running = self.controller.is_running()
        self.start_btn.configure(state=("disabled" if running else "normal"))
        self.stop_btn.configure(state=("normal" if running else "disabled"))
        self.apply_tune_btn.configure(state=("normal" if running else "disabled"))

    def _poll_events(self) -> None:
        while True:
            try:
                kind, state, message = self.event_queue.get_nowait()
            except queue.Empty:
                break

            if kind == "status":
                title, detail = self._friendly_status(state, message)
                self._set_state(title, detail)
                if message:
                    self._append_log(f"{title}: {message}")
                else:
                    self._append_log(title)
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
    app = AgentApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
