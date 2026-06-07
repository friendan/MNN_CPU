@echo off
cd /d "%~dp0..\build_ocr_test_release"
ninja -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
copy /Y bin\OCR_Test.exe "%~dp0..\bin\OCR_Test.exe" >nul
echo [OK]
