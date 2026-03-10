import asyncio
from typing import Optional

from config import IDLE_AFTER_NOTIFY_MS
from monitors import Emotion, RoboEvent


class Dispatcher:
    def __init__(self, ble):
        self.ble = ble
        self.current_emotion = Emotion.IDLE
        self.current_priority = 0
        self.reset_task: Optional[asyncio.Task] = None

    async def auto_reset(self, delay_s: float):
        await asyncio.sleep(delay_s)
        self.current_emotion = Emotion.IDLE
        self.current_priority = 0
        if self.ble.is_connected:
            await self.ble.send_emotion(Emotion.IDLE)

    async def handle(self, event: RoboEvent):
        if event.priority < self.current_priority:
            return

        if self.reset_task and not self.reset_task.done():
            self.reset_task.cancel()

        self.current_emotion = event.emotion
        self.current_priority = event.priority

        if self.ble.is_connected:
            await self.ble.send_emotion(event.emotion)

        duration_ms = event.duration_ms if event.duration_ms > 0 else 0
        if duration_ms == 0 and event.priority >= 6:
            duration_ms = IDLE_AFTER_NOTIFY_MS
        if duration_ms > 0:
            self.reset_task = asyncio.create_task(self.auto_reset(duration_ms / 1000.0))
