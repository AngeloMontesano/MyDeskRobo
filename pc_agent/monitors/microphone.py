import asyncio
import logging

from config import MIC_APPS, MIC_PEAK_THRESHOLD, MIC_POLL_S
from monitors import Emotion, RoboEvent

LOG = logging.getLogger("microphone")

try:
    from pycaw.pycaw import AudioUtilities, IAudioMeterInformation

    HAS_PYCAW = True
except Exception:
    HAS_PYCAW = False


def _is_mic_active() -> bool:
    sessions = AudioUtilities.GetAllSessions()
    for session in sessions:
        proc = session.Process
        if not proc:
            continue
        if proc.name() not in MIC_APPS:
            continue
        meter = session._ctl.QueryInterface(IAudioMeterInformation)
        peak = meter.GetPeakValue()
        if peak > MIC_PEAK_THRESHOLD:
            return True
    return False


async def run(queue: asyncio.Queue):
    if not HAS_PYCAW:
        LOG.info("pycaw not installed, microphone monitor idle")
        while True:
            await asyncio.sleep(60)

    was_active = False
    while True:
        active = await asyncio.to_thread(_is_mic_active)
        if active and not was_active:
            await queue.put(
                RoboEvent(
                    emotion=Emotion.HAPPY,
                    priority=4,
                    duration_ms=0,
                    source="mic",
                    description="mic active",
                )
            )
        elif not active and was_active:
            await queue.put(
                RoboEvent(
                    emotion=Emotion.IDLE,
                    priority=1,
                    duration_ms=0,
                    source="mic",
                    description="mic inactive",
                )
            )
        was_active = active
        await asyncio.sleep(MIC_POLL_S)
