@echo off
call "%~dp0tools\pio.cmd" run -e aeropad %*
exit /b %errorlevel%
