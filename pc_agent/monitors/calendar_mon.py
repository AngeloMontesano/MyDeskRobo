import asyncio
import importlib.util
import logging
from datetime import datetime, timedelta

from config import CALENDAR_POLL_S, CALENDAR_WARN_MIN
from monitors import Emotion, RoboEvent

LOG = logging.getLogger("calendar")
HAS_WIN32 = importlib.util.find_spec("pythoncom") is not None and importlib.util.find_spec("win32com") is not None
_NOTIFIED = set()


def _calendar_events():
    import pythoncom
    import win32com.client

    pythoncom.CoInitialize()
    try:
        outlook = win32com.client.Dispatch("Outlook.Application")
        ns = outlook.GetNamespace("MAPI")
        cal = ns.GetDefaultFolder(9)
        items = cal.Items
        items.IncludeRecurrences = True
        items.Sort("[Start]")

        now = datetime.now()
        soon = now + timedelta(minutes=15)
        restriction = (
            f"[Start] >= '{now.strftime('%m/%d/%Y %H:%M')}' "
            f"AND [Start] <= '{soon.strftime('%m/%d/%Y %H:%M')}'"
        )
        filtered = items.Restrict(restriction)
        out = []
        item = filtered.GetFirst()
        while item is not None:
            sub = str(getattr(item, "Subject", "") or "")
            start = getattr(item, "Start", None)
            if start:
                out.append((sub, start))
            item = filtered.GetNext()
        return out
    finally:
        pythoncom.CoUninitialize()


async def run(queue: asyncio.Queue):
    if not HAS_WIN32:
        LOG.info("pywin32 not installed, calendar monitor idle")
        while True:
            await asyncio.sleep(60)

    while True:
        events = await asyncio.to_thread(_calendar_events)
        now = datetime.now()
        for subject, start in events:
            key = f"{subject}|{start}"
            # Outlook can return timezone-aware starts on some systems/profiles.
            if getattr(start, "tzinfo", None) is not None:
                now_cmp = datetime.now(start.tzinfo)
            else:
                now_cmp = now
            minutes = (start - now_cmp).total_seconds() / 60.0
            if key not in _NOTIFIED and minutes <= CALENDAR_WARN_MIN:
                _NOTIFIED.add(key)
                await queue.put(
                    RoboEvent(
                        emotion=Emotion.CALENDAR,
                        priority=8,
                        duration_ms=10000,
                        source="calendar",
                        description=f"meeting soon: {subject}",
                    )
                )
        await asyncio.sleep(CALENDAR_POLL_S)
