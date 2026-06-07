@echo off
cd /d "%~dp0..\build_mnn_release"
ninja -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo [OK]
