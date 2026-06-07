#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

/* 函数指针类型定义 */
typedef int      (__cdecl *FN_ocrInit)(int);
typedef void     (__cdecl *FN_ocrDestroy)(void);
typedef void*    (__cdecl *FN_ocrRecognizeFile)(const char*);
typedef void*    (__cdecl *FN_ocrRecognize)(const unsigned char*, int, int);
typedef void     (__cdecl *FN_ocrFreeResult)(void*);

/* 结果结构（必须与 DLL 中定义一致） */
typedef struct {
    char* text;
    float confidence;
    float box[8];
} OCRLine;

typedef struct {
    OCRLine* lines;
    int count;
} OCRResult;

int main(int argc, char* argv[]) {
    const char* image_path = "ocr.png";
    if (argc > 1) image_path = argv[1];

    /* 加载 DLL */
    HMODULE hDll = LoadLibraryA("MNN_dbg.dll");
    if (!hDll) {
        fprintf(stderr, "Failed to load MNN_dbg.dll\n");
        return -1;
    }

    FN_ocrInit             fnInit   = (FN_ocrInit)GetProcAddress(hDll, "MNN_ocrInit");
    FN_ocrDestroy          fnDestroy = (FN_ocrDestroy)GetProcAddress(hDll, "MNN_ocrDestroy");
    FN_ocrRecognizeFile    fnRecFile = (FN_ocrRecognizeFile)GetProcAddress(hDll, "MNN_ocrRecognizeFile");
    FN_ocrFreeResult       fnFree    = (FN_ocrFreeResult)GetProcAddress(hDll, "MNN_ocrFreeResult");

    if (!fnInit || !fnDestroy || !fnRecFile || !fnFree) {
        fprintf(stderr, "Failed to get function addresses\n");
        FreeLibrary(hDll);
        return -1;
    }

    printf("MNN OCR Test\n");
    printf("Image: %s\n\n", image_path);

    /* 初始化 */
    if (fnInit(4) != 0) {
        fprintf(stderr, "MNN_ocrInit failed\n");
        FreeLibrary(hDll);
        return -1;
    }

    /* 识别 */
    OCRResult* result = (OCRResult*)fnRecFile(image_path);
    if (result) {
        printf("Results: %d lines\n\n", result->count);
        for (int i = 0; i < result->count; i++) {
            printf("[%d] Conf: %.4f\n", i, result->lines[i].confidence);
            printf("    Box:");
            for (int j = 0; j < 8; j += 2)
                printf(" (%.0f, %.0f)", result->lines[i].box[j], result->lines[i].box[j + 1]);
            printf("\n");
            printf("    Text: %s\n\n", result->lines[i].text);
        }
        fnFree(result);
    }

    fnDestroy();
    FreeLibrary(hDll);
    printf("Done.\n");
    return 0;
}
