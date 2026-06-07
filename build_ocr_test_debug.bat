@echo off
cd /d "%~dp0."
call "%~dp0init_env.bat" || exit /b 1
cd /d "%~dp0build_ocr_test_debug"
ninja -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo [OK]
