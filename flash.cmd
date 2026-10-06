@echo off
call "%~dp0tools\pio.cmd" run -e aeropad -t upload %*
exit /b %errorlevel%
