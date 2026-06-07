@echo off
REM 初始化 VS 2026 x64 编译环境
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Failed to initialize VS 2026 x64 environment
    exit /b 1
)
