import asyncio
import logging
import re
from pathlib import Path

import aiofiles

from config import TEAMS_LOG_PATH, TEAMS_LOG_POLL_MS
from monitors import Emotion, RoboEvent

LOG = logging.getLogger("teams")

PATTERNS = {
    r"callingservice.*incoming call": (Emotion.CALL, 10, 0),
    r"chat notification.*unread": (Emotion.TEAMS_MSG, 6, 8000),
    r"calling.*callstate.*connected": (Emotion.SURPRISED, 8, 5000),
}


async def run(queue: asyncio.Queue):
    path = Path(TEAMS_LOG_PATH)
    if not path.exists():
        LOG.info("teams log not found: %s", path)
        while True:
            await asyncio.sleep(10)

    async with aiofiles.open(path, "r", encoding="utf-8", errors="ignore") as fh:
        await fh.seek(0, 2)
        while True:
            line = await fh.readline()
            if not line:
                await asyncio.sleep(TEAMS_LOG_POLL_MS / 1000.0)
                continue
            low = line.lower()
            for pattern, (emotion, priority, duration_ms) in PATTERNS.items():
                if re.search(pattern, low):
                    await queue.put(
                        RoboEvent(
                            emotion=emotion,
                            priority=priority,
                            duration_ms=duration_ms,
                            source="teams",
                            description=line.strip()[:120],
                        )
                    )
                    break
