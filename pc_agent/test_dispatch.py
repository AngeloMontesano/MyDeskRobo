import asyncio
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from monitors import Emotion, RoboEvent
from dispatch import Dispatcher


class _FakeBle:
    def __init__(self):
        self.sent = []
        self.is_connected = True

    async def send_emotion(self, emotion: int):
        self.sent.append(emotion)


class DispatchTests(unittest.IsolatedAsyncioTestCase):
    async def test_priority_blocks_lower(self):
        ble = _FakeBle()
        disp = Dispatcher(ble)

        hi = RoboEvent(emotion=Emotion.CALL, priority=10, duration_ms=0, source="t", description="")
        lo = RoboEvent(emotion=Emotion.HAPPY, priority=3, duration_ms=0, source="t", description="")
        await disp.handle(hi)
        await disp.handle(lo)

        self.assertEqual(ble.sent[-1], Emotion.CALL)

    async def test_auto_reset(self):
        ble = _FakeBle()
        disp = Dispatcher(ble)
        ev = RoboEvent(emotion=Emotion.EMAIL, priority=7, duration_ms=100, source="t", description="")
        await disp.handle(ev)
        await asyncio.sleep(0.2)
        self.assertIn(Emotion.IDLE, ble.sent)


if __name__ == "__main__":
    unittest.main()
