# MNN OCR DLL

基于 [MNN](https://github.com/alibaba/MNN) 推理引擎 + PP-OCRv5 的纯 CPU OCR 识别 DLL。

4 个模型文件直接嵌入 DLL 资源中，外部使用只需 **一个 DLL 文件**，通过 `LoadLibrary` 动态加载。

## 目录结构

```
MNN_CPU/
├── bin/                     ← 编译输出
│   ├── MNN.dll              Release DLL（含 OCR 引擎 + 模型资源）
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

## 编译步骤

在项目根目录按顺序执行：

```
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
OCR_Test_dbg.exe ocr.png
```

## API

| 函数 | 说明 |
|------|------|
| `int MNN_ocrInit(int num_thread)` | 初始化引擎（全局唯一，可重复调用） |
| `void MNN_ocrDestroy(void)` | 销毁引擎 |
| `MNN_OCRResult* MNN_ocrRecognize(const uint8_t* rgba, int w, int h)` | 从内存 RGBA 数据识别 |
| `MNN_OCRResult* MNN_ocrRecognizeFile(const char* path)` | 从图片文件识别 |
| `void MNN_ocrFreeResult(MNN_OCRResult* result)` | 释放结果内存 |

返回的 `MNN_OCRResult*` 由 DLL 内部分配，调用者必须通过 `MNN_ocrFreeResult` 释放。

### 结果结构

```c
typedef struct {
    char* text;       // 识别文本
    float confidence; // 置信度
    float box[8];     // 四点坐标 [x0,y0, x1,y1, x2,y2, x3,y3]
} MNN_OCRLine;

typedef struct {
    MNN_OCRLine* lines;
    int count;
} MNN_OCRResult;
```

## 在自己项目中使用

```c
#include <windows.h>

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

## 技术说明

### 编译环境
- 使用 **LLVM/ClangCL** 编译器（VS 2026 自带）
- Ninja 生成器 + cmake
- `-T ClangCL -A x64` 是 VS 生成器参数，Ninja 不支持。编译器选择写在 CMakeLists.txt 的 `project()` 之前

### 重复符号处理
MNN 源码中 `x86_x64/` 的 AVX2/SSE 优化实现与 `compute/` 的通用 fallback 定义了同名函数。通过 `/FORCE:MULTIPLE` 让 lld-link 允许重复符号（优化实现优先）

### 模型加载
`Module::load({}, {})` 自动识别输入输出名，无需手动指定

### CTC 解码
PP-OCRv5 的 blank 索引为 0（全角空格），同时 class_num-1 也是 blank。字符索引需要 `max_idx - 1`

## 注意事项

- 使用 `/MT` 静态 CRT 链接，部署时无需 VC 运行时库
- Debug DLL 后缀 `_dbg`，Release 无后缀
- 模型文件编译时嵌入 DLL 资源，运行时无需额外模型文件
