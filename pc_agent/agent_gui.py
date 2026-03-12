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

    def _run_thread(self, mode: str) -> None:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        stop_event = asyncio.Event()

        with self._lock:
            self._loop = loop
            self._stop_event = stop_event

        def status_callback(state: str, message: str) -> None:
            self.event_queue.put(("status", state, message))

        try:
            loop.run_until_complete(
                agent_runtime.run(
                    mode,
                    status_callback=status_callback,
                    stop_event=stop_event,
                )
            )
            self.event_queue.put(("stopped", "", ""))
        except Exception as exc:
            self.event_queue.put(("error", "runtime", str(exc)))
        finally:
            with self._lock:
                self._loop = None
                self._stop_event = None
                self._thread = None
            loop.close()


class AgentApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("DeskRobo PC Agent")
        self.root.geometry("560x420")
        self.root.minsize(520, 360)

        self.event_queue: "queue.Queue[tuple]" = queue.Queue()
        self.controller = AgentController(self.event_queue)

        self.mode_var = tk.StringVar(value="basic")
        self.state_var = tk.StringVar(value="Nicht gestartet")
        self.detail_var = tk.StringVar(value="")

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
