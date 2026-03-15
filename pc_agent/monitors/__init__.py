import asyncio
import logging
from dataclasses import dataclass

LOG = logging.getLogger("monitor")


@dataclass
class RoboEvent:
    event_name: str   # e.g. "PC_MAIL", "PC_CALL", "MIC_ACTIVE"
    priority: int
    duration_ms: int
    source: str
    description: str


async def safe_monitor(coro_func, queue: asyncio.Queue, name: str):
    while True:
        try:
            await coro_func(queue)
        except Exception as exc:
            LOG.error("[%s] error: %s, restart in 10s", name, exc)
            await asyncio.sleep(10)
