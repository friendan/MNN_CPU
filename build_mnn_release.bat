@echo off
cd /d "%~dp0."
call "%~dp0init_env.bat" || exit /b 1
cd /d "%~dp0build_mnn_release"
ninja -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo [OK]
