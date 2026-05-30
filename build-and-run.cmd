@echo off
REM Build and run the whole URemoteControl stack (Go relay + both C clients).
REM Double-click to run with defaults (Debug), or from a terminal pass options, e.g.:
REM     build-and-run.cmd -Configuration Release -Clean -KeepOpen
REM     build-and-run.cmd -BuildOnly
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-and-run.ps1" %*
echo.
pause
