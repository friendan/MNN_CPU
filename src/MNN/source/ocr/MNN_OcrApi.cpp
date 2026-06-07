//
//  MNN_OcrApi.cpp
//  MNN
//
//  PP-OCRv5 C API 实现
//

#include <MNN/MNN_OcrApi.h>
#include "MNN_OcrEngine.hpp"

#include <cstring>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

extern "C" {

// ============================================================
// MNN_ocrInit
// ============================================================
MNN_OCR_API int MNN_ocrInit(int num_thread) {
    return MNN::OCR::global_init(num_thread);
}

// ============================================================
// MNN_ocrDestroy
// ============================================================
MNN_OCR_API void MNN_ocrDestroy(void) {
    MNN::OCR::global_destroy();
}

// ============================================================
// MNN_ocrRecognize (内存 RGBA 数据)
// ============================================================
MNN_OCR_API MNN_OCRResult MNN_ocrRecognize(const uint8_t* image_data, int width, int height) {
    MNN_OCRResult result;
    result.lines = nullptr;
    result.count = 0;

    auto* engine = MNN::OCR::global_instance();
    if (!engine || !image_data || width <= 0 || height <= 0) return result;

    auto text_lines = engine->recognize(image_data, width, height);

    result.count = (int)text_lines.size();
    if (result.count > 0) {
        result.lines = new MNN_OCRLine[result.count];
        for (int i = 0; i < result.count; i++) {
            result.lines[i].text = _strdup(text_lines[i].text.c_str());
            result.lines[i].confidence = text_lines[i].confidence;
            memcpy(result.lines[i].box, text_lines[i].box, sizeof(float) * 8);
        }
    }
    return result;
}

// ============================================================
// MNN_ocrRecognizeFile (从文件加载)
// ============================================================
MNN_OCR_API MNN_OCRResult MNN_ocrRecognizeFile(const char* image_path) {
    MNN_OCRResult result;
    result.lines = nullptr;
    result.count = 0;

    if (!image_path) return result;

    int width, height, channels;
    uint8_t* img_data = stbi_load(image_path, &width, &height, &channels, 4);
    if (!img_data) {
        MNN_ERROR("[OCR] Failed to load image: %s\n", image_path);
        return result;
    }

    result = MNN_ocrRecognize(img_data, width, height);
    stbi_image_free(img_data);
    return result;
}

// ============================================================
// MNN_ocrFreeResult
// ============================================================
MNN_OCR_API void MNN_ocrFreeResult(MNN_OCRResult* result) {
    if (result && result->lines) {
        for (int i = 0; i < result->count; i++) {
            free(result->lines[i].text);
        }
        delete[] result->lines;
        result->lines = nullptr;
        result->count = 0;
    }
}

} // extern "C"
