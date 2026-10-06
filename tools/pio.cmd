@echo off
setlocal
set PYTHONUTF8=1
set PYTHONIOENCODING=utf-8
set "PIO=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
if not exist "%PIO%" (
  echo PlatformIO Core not found: %PIO%
  exit /b 1
)
cd /d "%~dp0.."
"%PIO%" %*
exit /b %errorlevel%
