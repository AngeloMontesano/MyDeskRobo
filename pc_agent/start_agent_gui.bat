@echo off
setlocal
cd /d "%~dp0"

if not exist ".venv\Scripts\pythonw.exe" (
  echo Virtuelle Umgebung fehlt.
  echo Bitte einmal ausfuehren:
  echo   python -m venv .venv
  echo   .venv\Scripts\python.exe -m pip install -r requirements.txt
  pause
  exit /b 1
)

start "DeskRobo Agent" ".venv\Scripts\pythonw.exe" "agent_gui.py"
exit /b 0
