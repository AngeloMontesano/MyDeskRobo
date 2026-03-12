import argparse
import asyncio
import logging
from typing import Callable, Optional

from ble_client import BleClient
from dispatch import Dispatcher
from monitors import Emotion, RoboEvent, safe_monitor
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
        # UI/log callbacks must never stop the agent runtime.
        pass


async def dispatch_loop(queue: asyncio.Queue, dispatcher: Dispatcher):
    while True:
        event = await queue.get()
        await dispatcher.handle(event)


async def run(mode: str, status_callback: StatusCallback = None, stop_event: Optional[asyncio.Event] = None):
    queue: asyncio.Queue[RoboEvent] = asyncio.Queue()
    ble = BleClient(status_callback=status_callback)
    dispatcher = Dispatcher(ble)

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

        # Graceful shutdown: send IDLE then disconnect.
        if ble.is_connected:
            await ble.send_emotion(Emotion.IDLE)
        await ble.disconnect()
        _emit_status(status_callback, "stopped", "Agent gestoppt")


def parse_args():
    p = argparse.ArgumentParser(description="DeskRobo PC Agent")
    p.add_argument(
        "--mode",
        choices=["all", "teams", "mic", "basic"],
        default="basic",
        help="basic=outlook+calendar, all=all monitors",
    )
    p.add_argument("--send", type=int, default=None, help="one-shot emotion byte")
    return p.parse_args()


async def main():
    args = parse_args()
    setup_logging()

    if args.send is not None:
        ble = BleClient()
        try:
            await ble.connect()
            await ble.send_emotion(args.send)
            await asyncio.sleep(0.5)
        finally:
            await ble.disconnect()
        return

    await run(args.mode)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
