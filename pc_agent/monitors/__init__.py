import asyncio
import logging
from dataclasses import dataclass

LOG = logging.getLogger("monitor")


@dataclass
class RoboEvent:
    emotion: int
    priority: int
    duration_ms: int
    source: str
    description: str


class Emotion:
    IDLE = 0
    HAPPY = 1
    SAD = 2
    ANGRY = 3
    SURPRISED = 4
    SLEEPY = 5
    WINK = 6
    LOVE = 7
    CALL = 10
    EMAIL = 11
    TEAMS_MSG = 12
    CALENDAR = 13
    NOTIFY = 14
    LOCK = 15
    WIFI = 16


async def safe_monitor(coro_func, queue: asyncio.Queue, name: str):
    while True:
        try:
            await coro_func(queue)
        except Exception as exc:
            LOG.error("[%s] error: %s, restart in 10s", name, exc)
            await asyncio.sleep(10)
