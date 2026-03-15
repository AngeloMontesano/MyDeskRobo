import argparse
import asyncio
import logging
from typing import Any, Callable, Dict, Optional

from ble_client import BleClient
from config import DEFAULT_EVENT_MAPPING
from dispatch import Dispatcher
from monitors import RoboEvent, safe_monitor
from monitors import calendar_mon, microphone, outlook, teams

StatusCallback = Optional[Callable[[str, str], None]]


def setup_logging():
    logging.basicConfig(
        level=logging.INFO,
        format="[%(asctime)s] %(name)s: %(message)s",
        datefmt="%H:%M:%S",
    )


def _emit_status(status_callback: StatusCallback, state: str, message: str = "") -> None:
    if not status_callback:
        return
    try:
        status_callback(state, message)
    except Exception:
        pass


async def dispatch_loop(queue: asyncio.Queue, dispatcher: Dispatcher):
    while True:
        event = await queue.get()
        await dispatcher.handle(event)


async def control_loop(
    control_queue: asyncio.Queue,
    ble: BleClient,
    dispatcher: Dispatcher,
    status_callback: StatusCallback,
):
    while True:
        cmd = await control_queue.get()
        if not isinstance(cmd, tuple) or len(cmd) < 1:
            continue
        name = cmd[0]
        try:
            if name == "tune" and len(cmd) == 3:
                _, key, value = cmd
                await ble.set_tuning(str(key), int(value))
            elif name == "style" and len(cmd) == 2:
                await ble.set_style(str(cmd[1]))
            elif name == "status_label" and len(cmd) == 2:
                await ble.set_status_label_visible(bool(cmd[1]))
            elif name == "backlight" and len(cmd) == 2:
                await ble.set_backlight(int(cmd[1]))
            elif name == "event" and len(cmd) == 2:
                await ble.push_event(str(cmd[1]))
            elif name == "emotion" and len(cmd) == 3:
                await ble.set_emotion_named(str(cmd[1]), int(cmd[2]))
            elif name == "eyes" and len(cmd) == 4:
                await ble.set_eyes(str(cmd[1]), str(cmd[2]), int(cmd[3]))
            elif name == "time_sync":
                await ble.send_time_sync()
            elif name == "tune_save":
                await ble.save_tuning()
            elif name == "tune_load":
                await ble.load_tuning()
            elif name == "factory_reset":
                await ble.factory_reset()
            elif name == "cmd" and len(cmd) == 2:
                await ble.send_command_payload(str(cmd[1]))
            elif name == "list_emotions":
                names = await ble.request_emotion_list()
                if names and status_callback:
                    _emit_status(status_callback, "data:emotion_list", ",".join(names))
            elif name == "update_mapping" and len(cmd) == 2:
                mapping: Dict = cmd[1]
                if isinstance(mapping, dict):
                    dispatcher.mapping = mapping
        except Exception:
            logging.exception("control command failed: %s", cmd)


async def run(
    mode: str,
    status_callback: StatusCallback = None,
    stop_event: Optional[asyncio.Event] = None,
    control_queue: Optional[asyncio.Queue[Any]] = None,
    mapping: Optional[Dict] = None,
):
    queue: asyncio.Queue[RoboEvent] = asyncio.Queue()
    ble = BleClient(status_callback=status_callback)
    dispatcher = Dispatcher(ble, mapping if mapping is not None else DEFAULT_EVENT_MAPPING.copy())

    _emit_status(status_callback, "starting", f"Agent startet im Modus: {mode}")

    tasks = [
        asyncio.create_task(ble.reconnect_loop()),
        asyncio.create_task(safe_monitor(outlook.run, queue, "outlook")),
        asyncio.create_task(safe_monitor(calendar_mon.run, queue, "calendar")),
    ]

    if mode in ("all", "teams"):
        tasks.append(asyncio.create_task(safe_monitor(teams.run, queue, "teams")))
    if mode in ("all", "mic"):
        tasks.append(asyncio.create_task(safe_monitor(microphone.run, queue, "mic")))

    tasks.append(asyncio.create_task(dispatch_loop(queue, dispatcher)))
    if control_queue is not None:
        tasks.append(asyncio.create_task(control_loop(control_queue, ble, dispatcher, status_callback)))

    try:
        if stop_event is None:
            await asyncio.gather(*tasks)
        else:
            stop_task = asyncio.create_task(stop_event.wait())
            done, _ = await asyncio.wait(
                tasks + [stop_task],
                return_when=asyncio.FIRST_COMPLETED,
            )
            if stop_task not in done:
                for task in done:
                    if task is stop_task:
                        continue
                    exc = task.exception()
                    if exc is not None:
                        raise exc
                await asyncio.gather(*tasks)
    except asyncio.CancelledError:
        pass
    finally:
        _emit_status(status_callback, "stopping", "Agent wird beendet")
        for task in tasks:
            task.cancel()
        await asyncio.gather(*tasks, return_exceptions=True)

        if ble.is_connected:
            await ble.set_emotion_named("IDLE", 1500)
        await ble.disconnect()
        _emit_status(status_callback, "stopped", "Agent gestoppt")


def parse_args():
    p = argparse.ArgumentParser(description="MyDeskRobo PC Agent")
    p.add_argument(
        "--mode",
        choices=["all", "teams", "mic", "basic"],
        default="basic",
        help="basic=outlook+calendar, all=all monitors",
    )
    p.add_argument("--send", type=int, default=None, help="one-shot emotion byte (legacy)")
    p.add_argument("--style", type=str, default=None, help="set eye style (EVE/EVE_CINEMATIC)")
    p.add_argument("--backlight", type=int, default=None, help="set backlight 0..100")
    p.add_argument("--status-label", choices=["on", "off"], default=None, help="show/hide bottom status label")
    p.add_argument("--event", type=str, default=None, help="trigger event by name (CALL/MAIL/TEAMS/LOUD/...)")
    p.add_argument("--emotion-name", type=str, default=None, help="set emotion by name (IDLE/HAPPY/SAD/ANGRY/WINK/XX/GLITCH/...)")
    p.add_argument("--emotion-hold", type=int, default=3500, help="hold duration for --emotion-name")
    p.add_argument("--eyes", type=str, default=None, help="set eye pair LEFT:RIGHT[:HOLD_MS]")
    p.add_argument("--tune", action="append", default=[], help="set tuning key=value (repeatable)")
    p.add_argument("--cmd", action="append", default=[], help="raw CMD payload, e.g. STYLE:EVE_CINEMATIC")
    return p.parse_args()


async def main():
    args = parse_args()
    setup_logging()

    if args.send is not None:
        ble = BleClient()
        try:
            await ble.connect()
            # Legacy: map byte to named emotion for compatibility
            await ble.send_command_payload(f"EMO:1:{args.send}")
            await asyncio.sleep(0.5)
        finally:
            await ble.disconnect()
        return

    has_control_cmd = any(
        [
            args.style is not None,
            args.backlight is not None,
            args.status_label is not None,
            args.event is not None,
            args.emotion_name is not None,
            args.eyes is not None,
            bool(args.tune),
            bool(args.cmd),
        ]
    )

    if has_control_cmd:
        ble = BleClient()
        try:
            await ble.connect()

            if args.style is not None:
                await ble.set_style(args.style)
            if args.backlight is not None:
                await ble.set_backlight(args.backlight)
            if args.status_label is not None:
                await ble.set_status_label_visible(args.status_label == "on")
            if args.event is not None:
                await ble.push_event(args.event)
            if args.emotion_name is not None:
                await ble.set_emotion_named(args.emotion_name, args.emotion_hold)
            if args.eyes is not None:
                parts = [part.strip() for part in args.eyes.split(":") if part.strip()]
                if len(parts) >= 2:
                    hold_ms = int(parts[2]) if len(parts) >= 3 else 5000
                    await ble.set_eyes(parts[0], parts[1], hold_ms)
                else:
                    logging.error("--eyes format invalid, expected LEFT:RIGHT[:HOLD_MS]")

            for item in args.tune:
                if "=" not in item:
                    logging.error("--tune format invalid: %s (expected key=value)", item)
                    continue
                key, raw_val = item.split("=", 1)
                try:
                    val = int(raw_val.strip())
                except ValueError:
                    logging.error("--tune value invalid: %s", item)
                    continue
                await ble.set_tuning(key.strip(), val)

            for payload in args.cmd:
                await ble.send_command_payload(payload)

            await asyncio.sleep(0.3)
        finally:
            await ble.disconnect()
        return

    await run(args.mode)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
