@echo off
cd /d "%~dp0.."
set "BUILD_DIR=build_ocr_test_release"
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -T ClangCL -A x64 ../src/test_ocr
if %ERRORLEVEL% neq 0 exit /b 1
echo [OK]
