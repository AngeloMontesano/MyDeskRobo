import asyncio
import logging
import time
from typing import Callable, Dict, Optional

from bleak import BleakClient, BleakScanner

from config import (
    BLE_ACK_TIMEOUT_S,
    BLE_CHAR_UUID,
    BLE_CONNECT_WAIT_S,
    BLE_DEVICE_NAME,
    BLE_DEVICE_NAME_ALIASES,
    BLE_HEARTBEAT_S,
    BLE_RECONNECT_DELAY_S,
    BLE_SCAN_TIMEOUT_S,
    BLE_SERVICE_UUID,
    BLE_TIME_SYNC_INTERVAL_S,
    BLE_TIME_SYNC_RETRY_S,
    BLE_WRITE_RETRIES,
    BLE_WRITE_RETRY_DELAY_S,
)

LOG = logging.getLogger("ble")


class BleClient:
    def __init__(self, status_callback: Optional[Callable[[str, str], None]] = None):
        self._client: Optional[BleakClient] = None
        self._lock = asyncio.Lock()
        self._connected_event = asyncio.Event()
        self._stop = False
        self._notify_enabled = False
        self._seq = 0
        self._ack_waiters: Dict[int, asyncio.Future] = {}
        self._pong_waiters: Dict[int, asyncio.Future] = {}
        self._status_callback = status_callback
        self._next_time_sync_monotonic = 0.0

    @property
    def is_connected(self) -> bool:
        return bool(self._client and self._client.is_connected)

    def _emit_status(self, state: str, message: str = "") -> None:
        cb = self._status_callback
        if not cb:
            return
        try:
            cb(state, message)
        except Exception:
            # Status callback failures must never break transport.
            pass

    def _next_seq(self) -> int:
        self._seq = (self._seq + 1) & 0xFF
        if self._seq == 0:
            self._seq = 1
        return self._seq

    def _resolve_waiter(self, waiters: Dict[int, asyncio.Future], seq: int, value: bool) -> None:
        fut = waiters.pop(seq, None)
        if fut and not fut.done():
            fut.set_result(value)

    def _clear_waiters(self) -> None:
        for waiters in (self._ack_waiters, self._pong_waiters):
            for fut in waiters.values():
                if not fut.done():
                    fut.set_result(False)
            waiters.clear()

    def _on_notify(self, _sender, data: bytearray) -> None:
        text = bytes(data).decode("utf-8", errors="ignore").strip()
        if not text:
            return

        # Protocol: ACK:<seq>:<1|0>, PONG:<seq>
        if text.startswith("ACK:"):
            parts = text.split(":", 2)
            if len(parts) == 3:
                try:
                    seq = int(parts[1]) & 0xFF
                    ok = parts[2].strip().upper() in ("1", "OK", "TRUE")
                    self._resolve_waiter(self._ack_waiters, seq, ok)
                except ValueError:
                    pass
            return

        if text.startswith("PONG:"):
            parts = text.split(":", 1)
            if len(parts) == 2:
                try:
                    seq = int(parts[1]) & 0xFF
                    self._resolve_waiter(self._pong_waiters, seq, True)
                except ValueError:
                    pass

    async def _find_device(self):
        service_uuid = BLE_SERVICE_UUID.lower()
        names = [BLE_DEVICE_NAME] + list(BLE_DEVICE_NAME_ALIASES)

        devices = await BleakScanner.discover(timeout=BLE_SCAN_TIMEOUT_S, return_adv=True)

        # Prefer exact service match, then fall back to known names.
        for dev, adv in devices.values():
            uuids = [u.lower() for u in (adv.service_uuids or [])]
            if service_uuid in uuids:
                return dev

        for name in names:
            for dev, _ in devices.values():
                if dev.name == name:
                    return dev

        return None

    async def wait_connected(self, timeout_s: float = BLE_CONNECT_WAIT_S) -> bool:
        if self.is_connected:
            return True
        try:
            await asyncio.wait_for(self._connected_event.wait(), timeout=timeout_s)
            return self.is_connected
        except asyncio.TimeoutError:
            return False

    async def connect(self) -> None:
        # pywin32/pythoncom can put the main thread into STA, which breaks
        # Bleak WinRT callbacks in console agents. Reset to MTA-compatible state.
        try:
            from bleak.backends.winrt.util import uninitialize_sta  # type: ignore

            uninitialize_sta()
        except Exception:
            pass

        self._emit_status("searching", "Suche nach DeskRobo...")
        dev = await self._find_device()
        if not dev:
            self._emit_status("not_found", "DeskRobo nicht gefunden")
            raise RuntimeError(
                f"BLE device not found (service={BLE_SERVICE_UUID}, names={BLE_DEVICE_NAME}/{BLE_DEVICE_NAME_ALIASES})"
            )

        self._emit_status("connecting", f"Verbinde mit {dev.name}")
        self._client = BleakClient(dev, disconnected_callback=self._on_disconnected)
        await self._client.connect()

        self._notify_enabled = False
        try:
            await self._client.start_notify(BLE_CHAR_UUID, self._on_notify)
            self._notify_enabled = True
            LOG.info("notifications enabled")
        except Exception as exc:
            LOG.warning("notify unavailable, fallback to write-only mode: %s", exc)

        self._connected_event.set()
        self._emit_status("connected", f"Verbunden: {dev.name}")
        LOG.info("connected: %s (%s)", dev.name, dev.address)

    def _on_disconnected(self, _client) -> None:
        self._connected_event.clear()
        self._notify_enabled = False
        self._clear_waiters()
        self._emit_status("disconnected", "BLE getrennt")
        LOG.warning("BLE disconnected")

    async def _write_text(self, text: str) -> None:
        if not self.is_connected or not self._client:
            raise RuntimeError("BLE not connected")
        await self._client.write_gatt_char(BLE_CHAR_UUID, text.encode("utf-8"), response=True)

    async def send_ping(self) -> bool:
        if not self.is_connected:
            return False
        if not self._notify_enabled:
            return True

        seq = self._next_seq()
        fut = asyncio.get_running_loop().create_future()
        self._pong_waiters[seq] = fut

        try:
            await self._write_text(f"PING:{seq}")
            return bool(await asyncio.wait_for(fut, timeout=BLE_ACK_TIMEOUT_S))
        except Exception as exc:
            LOG.warning("heartbeat failed: %s", exc)
            return False
        finally:
            self._pong_waiters.pop(seq, None)

    async def send_time_sync(self, epoch_s: Optional[int] = None) -> bool:
        epoch_s = int(time.time() if epoch_s is None else epoch_s)
        if epoch_s <= 0:
            return False

        async with self._lock:
            if not await self.wait_connected():
                LOG.warning("skip time sync (not connected)")
                return False

            seq = self._next_seq()
            fut: Optional[asyncio.Future] = None

            try:
                if self._notify_enabled:
                    fut = asyncio.get_running_loop().create_future()
                    self._ack_waiters[seq] = fut

                await self._write_text(f"TIME:{seq}:{epoch_s}")

                if self._notify_enabled and fut is not None:
                    ok = bool(await asyncio.wait_for(fut, timeout=BLE_ACK_TIMEOUT_S))
                    if ok:
                        LOG.info("tx time sync epoch=%d seq=%d ack=ok", epoch_s, seq)
                    else:
                        LOG.warning("tx time sync epoch=%d seq=%d ack=nack", epoch_s, seq)
                    return ok

                LOG.info("tx time sync epoch=%d seq=%d (notify off)", epoch_s, seq)
                return True
            except Exception as exc:
                LOG.warning("time sync failed: %s", exc)
                self._connected_event.clear()
                return False
            finally:
                self._ack_waiters.pop(seq, None)

    async def send_emotion(self, emotion: int) -> bool:
        emotion = int(emotion) & 0xFF

        for attempt in range(1, BLE_WRITE_RETRIES + 1):
            async with self._lock:
                if not await self.wait_connected():
                    LOG.warning("drop emotion=%d (not connected)", emotion)
                    return False

                try:
                    if self._notify_enabled:
                        seq = self._next_seq()
                        fut = asyncio.get_running_loop().create_future()
                        self._ack_waiters[seq] = fut

                        await self._write_text(f"EMO:{seq}:{emotion}")
                        ok = bool(await asyncio.wait_for(fut, timeout=BLE_ACK_TIMEOUT_S))
                        if ok:
                            LOG.info("tx emotion=%d seq=%d ack=ok", emotion, seq)
                            return True

                        LOG.warning("tx emotion=%d seq=%d ack=nack", emotion, seq)
                    else:
                        # Legacy fallback for older firmware.
                        await self._client.write_gatt_char(
                            BLE_CHAR_UUID,
                            bytes([emotion]),
                            response=True,
                        )
                        LOG.info("tx emotion=%d (legacy mode)", emotion)
                        return True
                except Exception as exc:
                    LOG.warning(
                        "BLE write attempt %d/%d failed: %s",
                        attempt,
                        BLE_WRITE_RETRIES,
                        exc,
                    )
                    self._connected_event.clear()
                finally:
                    if self._notify_enabled:
                        self._ack_waiters.pop(locals().get("seq", -1), None)

            if attempt < BLE_WRITE_RETRIES:
                await asyncio.sleep(BLE_WRITE_RETRY_DELAY_S * attempt)

        return False

    async def disconnect(self) -> None:
        self._stop = True
        if self._client and self._client.is_connected:
            try:
                await self._client.disconnect()
            except Exception:
                pass
        self._connected_event.clear()
        self._notify_enabled = False
        self._clear_waiters()

    async def reconnect_loop(self):
        while not self._stop:
            try:
                if not self.is_connected:
                    await self.connect()
                    self._next_time_sync_monotonic = 0.0
                    await asyncio.sleep(0.2)
                    continue

                now = time.monotonic()
                if now >= self._next_time_sync_monotonic:
                    synced = await self.send_time_sync()
                    self._next_time_sync_monotonic = now + (
                        BLE_TIME_SYNC_INTERVAL_S if synced else BLE_TIME_SYNC_RETRY_S
                    )

                if not await self.send_ping():
                    self._emit_status("heartbeat_timeout", "Heartbeat-Timeout")
                    LOG.warning("heartbeat timeout, forcing reconnect")
                    if self._client and self._client.is_connected:
                        await self._client.disconnect()

                await asyncio.sleep(BLE_HEARTBEAT_S)
            except Exception as exc:
                self._emit_status("retry", f"Neuversuch in {BLE_RECONNECT_DELAY_S}s: {exc}")
                LOG.warning("reconnect retry in %ss: %s", BLE_RECONNECT_DELAY_S, exc)
                await asyncio.sleep(BLE_RECONNECT_DELAY_S)