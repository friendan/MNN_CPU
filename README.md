# MNN OCR DLL

基于 [MNN](https://github.com/alibaba/MNN) 推理引擎的 PP-OCRv5 纯 CPU 推理 DLL。

4 个模型文件直接嵌入 DLL 资源中，外部使用只需 **一个 DLL 文件**，通过 `LoadLibrary` 动态加载。

## 目录结构

```
MNN_CPU/
├── bin/                     ← 编译输出
│   ├── MNN.dll              Release DLL
│   ├── MNN_dbg.dll          Debug DLL
│   ├── MNN.lib / MNN_dbg.lib   导入库
│   ├── OCR_Test.exe         测试程序 Release
│   └── OCR_Test_dbg.exe     测试程序 Debug
├── src/
│   ├── MNN/                 MNN 推理引擎源码
│   └── test_ocr/            测试工程
├── init_env.bat             初始化 VS 编译环境
├── create_xxx_sln.bat       生成工程脚本
├── build_xxx.bat            编译脚本
└── README.md
```

## 前置要求

- Visual Studio 2026，安装时勾选 **LLVM (ClangCL) 工具集**
- CMake 3.16+
- Ninja

## 编译

双击或命令行运行对应 bat 即可（不需要手动进 VS 命令提示符，`init_env.bat` 会自动初始化）：

```bat
REM ---- Debug ----
create_mnn_debug_sln.bat      生成工程 → build_mnn_debug/
build_mnn_debug.bat           编译     → bin/MNN_dbg.dll + bin/MNN_dbg.lib

create_ocr_test_debug_sln.bat 生成工程 → build_ocr_test_debug/
build_ocr_test_debug.bat      编译     → bin/OCR_Test_dbg.exe

REM ---- Release ----
create_mnn_release_sln.bat
build_mnn_release.bat

create_ocr_test_release_sln.bat
build_ocr_test_release.bat
```

### 运行测试

```bat
cd bin
OCR_Test_dbg.exe                使用 bin/ocr.png
OCR_Test_dbg.exe my_pic.png     指定图片
```

## 技术说明

### 为什么不用 `-T ClangCL`？

Ninja 生成器不支持 `-T`（toolset）和 `-A`（platform）参数，这两个是 Visual Studio 生成器专用的。所以我们把编译器选择写在 CMakeLists.txt 中：

```cmake
if(WIN32)
    set(CMAKE_C_COMPILER clang-cl)
    set(CMAKE_CXX_COMPILER clang-cl)
endif()
```

这样 Ninja 也能用上 LLVM/ClangCL。`-A x64` 也不需要，因为 `init_env.bat` 中 `vcvarsall.bat x64` 已经设好了 x64 环境。

### 为什么有 `/FORCE:MULTIPLE`？

MNN 的源码中，`x86_x64/` 目录下的 AVX2/SSE/AVX512 优化实现与 `compute/CommonOptFunction.cpp` 等通用 fallback 定义了同名函数。MSVC 的 link.exe 遇到重复符号会报错，lld-link 也同理。通过 `/FORCE:MULTIPLE` 让链接器允许重复符号，取第一个定义（即优化实现优先），fallback 被忽略。这保证了 AVX2 等优化的正常生效。

## 在自己项目中使用

只需 `MNN.dll` 一个文件，运行时 `LoadLibrary` 动态加载：

```c
#include <windows.h>
#include <stdio.h>

typedef int      (__cdecl *FN_ocrInit)(int);
typedef void     (__cdecl *FN_ocrDestroy)(void);
typedef void*    (__cdecl *FN_ocrRecognizeFile)(const char*);
typedef void     (__cdecl *FN_ocrFreeResult)(void*);

typedef struct { char* text; float confidence; float box[8]; } OCRLine;
typedef struct { OCRLine* lines; int count; } OCRResult;

int main() {
    HMODULE dll = LoadLibraryA("MNN.dll");

    FN_ocrInit          init  = (FN_ocrInit)GetProcAddress(dll, "MNN_ocrInit");
    FN_ocrRecognizeFile recog = (FN_ocrRecognizeFile)GetProcAddress(dll, "MNN_ocrRecognizeFile");
    FN_ocrFreeResult    free  = (FN_ocrFreeResult)GetProcAddress(dll, "MNN_ocrFreeResult");
    FN_ocrDestroy       dest  = (FN_ocrDestroy)GetProcAddress(dll, "MNN_ocrDestroy");

    init(4);
    OCRResult* r = (OCRResult*)recog("test.png");
    for (int i = 0; i < r->count; i++)
        printf("%s\n", r->lines[i].text);
    free(r);
    dest();
    FreeLibrary(dll);
    return 0;
}
```

## API

| 函数 | 说明 |
|------|------|
| `int MNN_ocrInit(int num_thread)` | 初始化引擎（全局唯一，可重复调用） |
| `void MNN_ocrDestroy(void)` | 销毁引擎 |
| `MNN_OCRResult* MNN_ocrRecognize(const uint8_t* rgba, int w, int h)` | 从内存 RGBA 数据识别 |
| `MNN_OCRResult* MNN_ocrRecognizeFile(const char* path)` | 从图片文件识别 |
| `void MNN_ocrFreeResult(MNN_OCRResult* result)` | 释放结果内存 |

> 返回的 `MNN_OCRResult*` 由 DLL 内部分配，调用者必须通过 `MNN_ocrFreeResult` 释放。

## 注意事项

- 使用 `/MT` 静态 CRT 链接，部署时无需 VC 运行时库
- Debug 版 DLL 加 `_dbg` 后缀，Release 版无后缀，两者可共存于 `bin/`
- MNN 源码中的代码页警告（C4819）是源文件编码问题，不影响功能
