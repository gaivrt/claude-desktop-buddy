@echo off
title Claude Buddy Voice
cd /d "%~dp0"
echo ============================================================
echo  Claude Buddy - Chinese voice input companion
echo  (FunASR loads ~50s on first start, then stays ready)
echo  Hold B on the Buddy and talk; text auto-types + sends.
echo  Close this window to stop.
echo ============================================================
echo.
uv run --with funasr --with torch --with torchaudio --with pyserial python -u companion\main.py
echo.
echo Companion exited. Press any key to close.
pause >nul
