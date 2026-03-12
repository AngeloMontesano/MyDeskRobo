@echo off
setlocal
cd /d "%~dp0"

if not exist ".venv\Scripts\python.exe" (
  echo Virtuelle Umgebung fehlt.
  echo Bitte einmal ausfuehren:
  echo   python -m venv .venv
  echo   .venv\Scripts\python.exe -m pip install -r requirements.txt
  pause
  exit /b 1
)

".venv\Scripts\python.exe" -m pip install pyinstaller
if errorlevel 1 (
  echo PyInstaller konnte nicht installiert werden.
  pause
  exit /b 1
)

".venv\Scripts\python.exe" -m PyInstaller --noconfirm --onefile --windowed --name DeskRoboAgent agent_gui.py
if errorlevel 1 (
  echo EXE-Build fehlgeschlagen.
  pause
  exit /b 1
)

echo Fertig. EXE unter: dist\DeskRoboAgent.exe
pause
