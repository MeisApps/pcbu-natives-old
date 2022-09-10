@echo off
robocopy "%~dp0\win-pcbiounlock\x64\Debug" "%windir%\system32" "win-pcbiounlock.dll"
pause
