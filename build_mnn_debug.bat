@echo off
cd /d "%~dp0..\build_mnn_debug"
ninja -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo [OK]
