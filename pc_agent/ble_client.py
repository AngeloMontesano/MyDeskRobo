import asyncio
import logging
from typing import Optional

from bleak import BleakClient, BleakScanner

from config import BLE_CHAR_UUID, BLE_DEVICE_NAME, BLE_RECONNECT_DELAY_S

LOG = logging.getLogger("ble")


class BleClient:
    def __init__(self):
        self._client: Optional[BleakClient] = None
        self._lock = asyncio.Lock()
        self._connected_event = asyncio.Event()
        self._stop = False

    @property
    def is_connected(self) -> bool:
        return bool(self._client and self._client.is_connected)

    async def connect(self) -> None:
        # pywin32/pythoncom can put the main thread into STA, which breaks
        # Bleak WinRT callbacks in console agents. Reset to MTA-compatible state.
        try:
            from bleak.backends.winrt.util import uninitialize_sta  # type: ignore

            uninitialize_sta()
        except Exception:
            pass

        dev = await BleakScanner.find_device_by_name(BLE_DEVICE_NAME, timeout=12.0)
        if not dev:
            raise RuntimeError(f"BLE device '{BLE_DEVICE_NAME}' not found")
        self._client = BleakClient(dev, disconnected_callback=self._on_disconnected)
        await self._client.connect()
        self._connected_event.set()
        LOG.info("connected: %s (%s)", dev.name, dev.address)

    def _on_disconnected(self, _client):
        self._connected_event.clear()
        LOG.warning("BLE disconnected")

    async def send_emotion(self, emotion: int) -> None:
        async with self._lock:
            if not self.is_connected:
                return
            try:
                # Use write-with-response for reliability on Windows BLE stacks.
                await self._client.write_gatt_char(BLE_CHAR_UUID, bytes([emotion & 0xFF]), response=True)
                LOG.info("tx emotion=%d", emotion)
            except Exception as exc:
                LOG.error("BLE write failed: %s", exc)
                self._connected_event.clear()

    async def disconnect(self) -> None:
        self._stop = True
        if self._client and self._client.is_connected:
            await self._client.disconnect()
        self._connected_event.clear()

    async def reconnect_loop(self):
        while not self._stop:
            try:
                if not self.is_connected:
                    await self.connect()
                await self._connected_event.wait()
                await asyncio.sleep(0.2)
            except Exception as exc:
                LOG.warning("reconnect retry in %ss: %s", BLE_RECONNECT_DELAY_S, exc)
                await asyncio.sleep(BLE_RECONNECT_DELAY_S)
