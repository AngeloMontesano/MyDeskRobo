from pathlib import Path

# BLE
BLE_DEVICE_NAME = "DeskRobot"
BLE_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
BLE_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLE_RECONNECT_DELAY_S = 5

# Event timings
IDLE_AFTER_NOTIFY_MS = 8000
CALENDAR_WARN_MIN = 5

# Polling
TEAMS_LOG_POLL_MS = 300
OUTLOOK_POLL_S = 10
CALENDAR_POLL_S = 60
MIC_POLL_S = 1

# Microphone
MIC_PEAK_THRESHOLD = 0.01
MIC_APPS = ["Teams.exe", "ms-teams.exe", "zoom.exe"]

# Teams log path
TEAMS_LOG_PATH = Path.home() / "AppData" / "Roaming" / "Microsoft" / "Teams" / "logs.txt"
