@echo off
cd /d "%~dp0."
call "%~dp0init_env.bat" || exit /b 1
set "BUILD_DIR=build_mnn_debug"
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ../src/MNN
if %ERRORLEVEL% neq 0 exit /b 1
echo [OK]
