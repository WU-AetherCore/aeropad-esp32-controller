@echo off
call "%~dp0tools\pio.cmd" device monitor -p COM11 -b 115200 %*
exit /b %errorlevel%
