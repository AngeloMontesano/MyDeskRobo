import asyncio
from typing import Dict, Optional

from config import DEFAULT_EVENT_MAPPING, IDLE_AFTER_NOTIFY_MS
from monitors import RoboEvent


class Dispatcher:
    def __init__(self, ble, mapping: Optional[Dict] = None):
        self.ble = ble
        self.mapping: Dict = mapping if mapping is not None else DEFAULT_EVENT_MAPPING.copy()
        self.current_priority = 0
        self.reset_task: Optional[asyncio.Task] = None

    async def auto_reset(self, delay_s: float):
        await asyncio.sleep(delay_s)
        self.current_priority = 0
        await self.ble.set_emotion_named("IDLE", 1500)

    async def handle(self, event: RoboEvent):
        entry = self.mapping.get(event.event_name)
        if not entry:
            return

        emotion_name = entry.get("emotion", "IDLE")
        priority = entry.get("priority", event.priority)
        duration_ms = entry.get("duration_ms", event.duration_ms)

        if priority < self.current_priority:
            return

        if self.reset_task and not self.reset_task.done():
            self.reset_task.cancel()

        self.current_priority = priority

        hold_ms = duration_ms if duration_ms > 0 else 3500
        await self.ble.set_emotion_named(emotion_name, hold_ms)

        if duration_ms > 0:
            self.reset_task = asyncio.create_task(self.auto_reset(duration_ms / 1000.0))
        elif priority >= 6:
            self.reset_task = asyncio.create_task(self.auto_reset(IDLE_AFTER_NOTIFY_MS / 1000.0))
