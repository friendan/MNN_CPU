#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "MnnOcr.hpp"

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);

    const char* image_path = "ocr.png";
    if (argc > 1) image_path = argv[1];

    MnnOcr ocr;
    if (!ocr.load()) {
        fprintf(stderr, "Failed to load MNN.dll\n");
        return -1;
    }
    if (!ocr.init(4)) {
        fprintf(stderr, "OCR init failed\n");
        return -1;
    }

    printf("MNN OCR Test\n");
    printf("Image: %s\n\n", image_path);

    auto result = ocr.recognizeFile(image_path);
    printf("Results: %zu lines\n\n", result.lines.size());
    for (size_t i = 0; i < result.lines.size(); i++) {
        printf("[%zu] Conf: %.4f\n", i, result.lines[i].confidence);
        printf("    Box:");
        for (int j = 0; j < 8; j += 2)
            printf(" (%.0f, %.0f)", result.lines[i].box[j], result.lines[i].box[j + 1]);
        printf("\n");
        printf("    Text: %s\n\n", result.lines[i].text.c_str());
    }

    ocr.destroy();
    printf("Done.\n");
    return 0;
}
