#ifndef MNN_OCR_HPP
#define MNN_OCR_HPP

#include <string>
#include <vector>
#include <windows.h>

/**
 * MNN OCR 封装类
 *
 * 使用方法：
 *   1. 把 MnnOcr.hpp 和 MNN.dll（或 MNN_dbg.dll）放到你的工程中
 *   2. #include "MnnOcr.hpp"
 *   3. 使用 MnnOcr 类：
 *
 *      MnnOcr ocr;
 *      ocr.load();              // 自动加载 MNN_dbg.dll 或 MNN.dll
 *      ocr.init(4);             // 初始化 OCR 引擎（4线程）
 *
 *      // 从文件识别（返回完整结果，含文本框和置信度）
 *      auto r = ocr.recognizeFile("test.png");
 *      for (auto& line : r.lines)
 *          printf("%s\n", line.text.c_str());
 *
 *      // 从文件识别（直接返回文本，多行用\n分隔）
 *      std::string text = ocr.recognizeFileText("test.png");
 *
 *      // 或从内存 RGBA 数据识别
 *      auto r2 = ocr.recognize(rgba_data, width, height);
 *      std::string text2 = ocr.recognizeText(rgba_data, width, height);
 *
 *      ocr.destroy();           // 销毁引擎
 *
 * 注意：编译时链接 msvcrt.lib（/MT），运行时 MNN.dll 放在 exe 同目录
 */
class MnnOcr {
public:
    MnnOcr() : m_dll(nullptr), m_init(nullptr), m_destroy(nullptr),
               m_recog(nullptr), m_recog_file(nullptr), m_free(nullptr) {}

    ~MnnOcr() {
        destroy();
    }

    /** 加载 DLL（默认按顺序搜索 MNN_dbg.dll / MNN.dll） */
    bool load(const char* dll_name = nullptr) {
        if (m_dll) return true;

        // 先试默认名称，再试用户指定的名称
        const char* defaults[] = { "MNN_dbg.dll", "MNN.dll", nullptr };
        for (int i = 0; defaults[i]; i++) {
            m_dll = LoadLibraryA(defaults[i]);
            if (m_dll) break;
        }
        if (!m_dll && dll_name) {
            m_dll = LoadLibraryA(dll_name);
        }
        if (!m_dll) return false;

        m_init        = reinterpret_cast<FN_ocrInit>(GetProcAddress(m_dll, "MNN_ocrInit"));
        m_destroy     = reinterpret_cast<FN_ocrDestroy>(GetProcAddress(m_dll, "MNN_ocrDestroy"));
        m_recog       = reinterpret_cast<FN_ocrRecognize>(GetProcAddress(m_dll, "MNN_ocrRecognize"));
        m_recog_file  = reinterpret_cast<FN_ocrRecognizeFile>(GetProcAddress(m_dll, "MNN_ocrRecognizeFile"));
        m_free        = reinterpret_cast<FN_ocrFreeResult>(GetProcAddress(m_dll, "MNN_ocrFreeResult"));

        if (!m_init || !m_destroy || !m_recog || !m_recog_file || !m_free) {
            FreeLibrary(m_dll);
            m_dll = nullptr;
            return false;
        }
        return true;
    }

    /** 初始化 OCR 引擎 */
    bool init(int num_thread = 4) {
        if (!m_dll) return false;
        return m_init(num_thread) == 0;
    }

    /** 销毁引擎 */
    void destroy() {
        if (m_destroy) m_destroy();
        if (m_dll) { FreeLibrary(m_dll); m_dll = nullptr; }
    }

    /** 识别单行文本 */
    struct Line {
        std::string text;
        float confidence = 0;
        float box[8] = {};
    };

    /** 识别结果 */
    struct Result {
        std::vector<Line> lines;
    };

    /** 从文件识别，返回完整结果 */
    Result recognizeFile(const char* image_path) {
        Result r;
        if (!m_dll || !m_recog_file) return r;
        OCRResult* raw = (OCRResult*)m_recog_file(image_path);
        if (raw) {
            for (int i = 0; i < raw->count; i++) {
                Line line;
                line.text = raw->lines[i].text ? raw->lines[i].text : "";
                line.confidence = raw->lines[i].confidence;
                memcpy(line.box, raw->lines[i].box, sizeof(float) * 8);
                r.lines.push_back(line);
            }
            m_free(raw);
        }
        return r;
    }

    /** 从文件识别，只返回拼接后的文本 */
    std::string recognizeFileText(const char* image_path) {
        auto r = recognizeFile(image_path);
        std::string s;
        for (auto& line : r.lines) {
            if (!s.empty()) s += "\n";
            s += line.text;
        }
        return s;
    }

    /** 从内存 RGBA 数据识别，返回完整结果 */
    Result recognize(const unsigned char* rgba, int w, int h) {
        Result r;
        if (!m_dll || !m_recog) return r;
        OCRResult* raw = (OCRResult*)m_recog(rgba, w, h);
        if (raw) {
            for (int i = 0; i < raw->count; i++) {
                Line line;
                line.text = raw->lines[i].text ? raw->lines[i].text : "";
                line.confidence = raw->lines[i].confidence;
                memcpy(line.box, raw->lines[i].box, sizeof(float) * 8);
                r.lines.push_back(line);
            }
            m_free(raw);
        }
        return r;
    }

    /** 从内存 RGBA 数据识别，只返回拼接后的文本 */
    std::string recognizeText(const unsigned char* rgba, int w, int h) {
        auto r = recognize(rgba, w, h);
        std::string s;
        for (auto& line : r.lines) {
            if (!s.empty()) s += "\n";
            s += line.text;
        }
        return s;
    }

private:
    HMODULE m_dll;

    typedef int      (__cdecl *FN_ocrInit)(int);
    typedef void     (__cdecl *FN_ocrDestroy)();
    typedef void*    (__cdecl *FN_ocrRecognize)(const unsigned char*, int, int);
    typedef void*    (__cdecl *FN_ocrRecognizeFile)(const char*);
    typedef void     (__cdecl *FN_ocrFreeResult)(void*);

    typedef struct {
        char* text;
        float confidence;
        float box[8];
    } OCRLine;

    typedef struct {
        OCRLine* lines;
        int count;
    } OCRResult;

    FN_ocrInit             m_init;
    FN_ocrDestroy          m_destroy;
    FN_ocrRecognize        m_recog;
    FN_ocrRecognizeFile    m_recog_file;
    FN_ocrFreeResult       m_free;
};

#endif // MNN_OCR_HPP
