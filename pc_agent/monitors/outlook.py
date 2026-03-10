import asyncio
import importlib.util
import logging

from config import OUTLOOK_POLL_S
from monitors import Emotion, RoboEvent

LOG = logging.getLogger("outlook")
HAS_WIN32 = importlib.util.find_spec("pythoncom") is not None and importlib.util.find_spec("win32com") is not None


class _State:
    last_count = -1
    last_entry_id = ""


def _snapshot():
    import pythoncom
    import win32com.client

    pythoncom.CoInitialize()
    try:
        outlook = win32com.client.Dispatch("Outlook.Application")
        ns = outlook.GetNamespace("MAPI")
        inbox = ns.GetDefaultFolder(6)
        items = inbox.Items
        count = int(items.Count)
        entry_id = ""

        items.Sort("[ReceivedTime]", True)
        msg = items.GetFirst()
        while msg is not None:
            mclass = str(getattr(msg, "MessageClass", "") or "")
            if mclass.startswith("IPM.Note"):
                entry_id = str(getattr(msg, "EntryID", "") or "")
                break
            msg = items.GetNext()
        return count, entry_id
    finally:
        pythoncom.CoUninitialize()


async def run(queue: asyncio.Queue):
    if not HAS_WIN32:
        LOG.info("pywin32 not installed, outlook monitor idle")
        while True:
            await asyncio.sleep(60)

    while True:
        count, entry_id = await asyncio.to_thread(_snapshot)
        if _State.last_count < 0:
            _State.last_count = count
            _State.last_entry_id = entry_id
        else:
            if count > _State.last_count or (entry_id and entry_id != _State.last_entry_id):
                await queue.put(
                    RoboEvent(
                        emotion=Emotion.EMAIL,
                        priority=7,
                        duration_ms=8000,
                        source="outlook",
                        description=f"inbox change count={count}",
                    )
                )
                _State.last_entry_id = entry_id
            _State.last_count = count

        await asyncio.sleep(OUTLOOK_POLL_S)
