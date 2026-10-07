@echo off
call "%~dp0build.bat" Win32 %*
exit /b %errorlevel%
