//
//  MNN_OcrEngine.cpp
//  MNN
//
//  PP-OCRv5 纯 CPU 推理引擎，模型从 Windows 资源中加载
//

#include "MNN_OcrEngine.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace MNN {
namespace OCR {

// ============================================================
// 工具函数
// ============================================================

static float point_distance(float x1, float y1, float x2, float y2) {
    return std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

// ============================================================
// 从 Windows 资源加载二进制数据
// ============================================================
// 链接器生成的 DLL 基地址，可作为 HMODULE 使用
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define DLL_HINSTANCE ((HMODULE)&__ImageBase)

bool load_resource(int res_id, const char* res_type, std::vector<uint8_t>& out) {
    HMODULE hMod = DLL_HINSTANCE;
    HRSRC hRsrc = FindResourceW(hMod, MAKEINTRESOURCEW(res_id), L"MNN_MODEL");
    if (!hRsrc) return false;

    HGLOBAL hGlobal = LoadResource(hMod, hRsrc);
    if (!hGlobal) return false;

    DWORD size = SizeofResource(hMod, hRsrc);
    const uint8_t* data = (const uint8_t*)LockResource(hGlobal);
    if (!data || size == 0) return false;

    out.assign(data, data + size);
    return true;
}

// ============================================================
// OCR 引擎
// ============================================================

OcrEngine::OcrEngine() {}

OcrEngine::~OcrEngine() {
    // shared_ptr 自动释放
}

bool OcrEngine::init(int num_thread) {
    if (num_thread <= 0) num_thread = 4;
    m_num_thread = num_thread;

    // 1. 从资源加载字典
    std::vector<uint8_t> dict_data;
    if (!load_resource(IDR_DICT_FILE, "MNN_MODEL", dict_data)) {
        MNN_ERROR("[OCR] Failed to load dict resource\n");
        return false;
    }
    if (!load_dict_from_memory(dict_data)) {
        MNN_ERROR("[OCR] Failed to parse dict\n");
        return false;
    }
    MNN_PRINT("[OCR] Dict loaded: %zu entries\n", m_dict.size());

    // 2. 从资源加载检测模型
    {
        std::vector<uint8_t> model_data;
        if (!load_resource(IDR_DET_MODEL, "MNN_MODEL", model_data)) {
            MNN_ERROR("[OCR] Failed to load detection model resource\n");
            return false;
        }
        MNN::ScheduleConfig sched;
        sched.type = MNN_FORWARD_CPU;
        sched.numThread = m_num_thread;
        auto rtMgr = std::shared_ptr<MNN::Express::Executor::RuntimeManager>(
            MNN::Express::Executor::RuntimeManager::createRuntimeManager(sched));

        MNN::Express::Module::Config config;
        config.shapeMutable = true;
        config.rearrange = true;

        // 尝试多种输入/输出名组合加载检测模型
        std::vector<std::string> inputs = {"x"};
        std::vector<std::string> outputs = {"sigmoid_0.tmp_0", "tmp_17"};
        m_det_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
        if (!m_det_module) {
            outputs = {"output"};
            m_det_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
        }
        if (!m_det_module) {
            inputs = {"input"}; outputs = {"output"};
            m_det_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
        }
        if (!m_det_module) {
            MNN_ERROR("[OCR] Failed to load detection model\n");
            return false;
        }
        MNN_PRINT("[OCR] Detection model loaded\n");
    }

    // 3. 从资源加载识别模型
    {
        std::vector<uint8_t> model_data;
        if (!load_resource(IDR_REC_MODEL, "MNN_MODEL", model_data)) {
            MNN_ERROR("[OCR] Failed to load recognition model resource\n");
            return false;
        }
        MNN::ScheduleConfig sched;
        sched.type = MNN_FORWARD_CPU;
        sched.numThread = m_num_thread;
        auto rtMgr = std::shared_ptr<MNN::Express::Executor::RuntimeManager>(
            MNN::Express::Executor::RuntimeManager::createRuntimeManager(sched));

        MNN::Express::Module::Config config;
        config.shapeMutable = false;
        config.rearrange = true;

        std::vector<std::string> inputs = {"x"};
        std::vector<std::string> outputs = {"save_infer_model/scale_0.tmp_0"};
        m_rec_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
        if (!m_rec_module) {
            outputs = {"softmax_0.tmp_0", "output"};
            m_rec_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
        }
        if (!m_rec_module) {
            inputs = {"input"}; outputs = {"output"};
            m_rec_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
        }
        if (!m_rec_module) {
            MNN_ERROR("[OCR] Failed to load recognition model\n");
            return false;
        }
        MNN_PRINT("[OCR] Recognition model loaded\n");
    }

    // 4. 从资源加载方向分类模型
    {
        std::vector<uint8_t> model_data;
        if (!load_resource(IDR_CLS_MODEL, "MNN_MODEL", model_data)) {
            MNN_PRINT("[OCR] Warning: Classification model not embedded, skip\n");
            // 可选，跳过不算失败
        } else {
            MNN::ScheduleConfig sched;
            sched.type = MNN_FORWARD_CPU;
            sched.numThread = m_num_thread;
            auto rtMgr = std::shared_ptr<MNN::Express::Executor::RuntimeManager>(
                MNN::Express::Executor::RuntimeManager::createRuntimeManager(sched));

            MNN::Express::Module::Config config;
            config.shapeMutable = false;
            config.rearrange = true;

            std::vector<std::string> inputs = {"x"};
            std::vector<std::string> outputs = {"save_infer_model/scale_0.tmp_0"};
            m_cls_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
            if (!m_cls_module) {
                inputs = {"input"}; outputs = {"output"};
                m_cls_module.reset(MNN::Express::Module::load(inputs, outputs, model_data.data(), model_data.size(), rtMgr, &config));
            }
            if (m_cls_module) {
                MNN_PRINT("[OCR] Classification model loaded\n");
            }
        }
    }

    MNN_PRINT("[OCR] Engine initialized successfully!\n");
    return true;
}

bool OcrEngine::load_dict_from_memory(const std::vector<uint8_t>& data) {
    m_dict.clear();
    std::string text((const char*)data.data(), data.size());
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            m_dict.push_back(line);
        }
    }
    return !m_dict.empty();
}

std::string OcrEngine::decode(const float* data, int seq_len, int class_num, float& confidence) {
    std::string result;
    confidence = 1.0f;
    int last_idx = -1;
    int blank_idx = class_num - 1;

    for (int t = 0; t < seq_len; t++) {
        int max_idx = 0;
        float max_val = data[t * class_num];
        for (int c = 1; c < class_num; c++) {
            float val = data[t * class_num + c];
            if (val > max_val) { max_val = val; max_idx = c; }
        }
        if (max_idx != blank_idx && max_idx != last_idx) {
            if (max_idx < (int)m_dict.size()) {
                result += m_dict[max_idx];
            }
        }
        if (max_idx != blank_idx) {
            confidence *= max_val;
        }
        last_idx = max_idx;
    }
    if (result.empty()) confidence = 0.0f;
    return result;
}

// ============================================================
// 检测
// ============================================================
std::vector<TextBox> OcrEngine::detect(const uint8_t* image_data, int width, int height) {
    std::vector<TextBox> results;
    if (!m_det_module) return results;

    int det_short_size = 960;
    float scale = std::min((float)det_short_size / width, (float)det_short_size / height);
    float max_scale = 1920.0f / std::max(width, height);
    scale = std::min(scale, max_scale);
    int resized_w = std::max(32, (int)(width * scale + 0.5f) / 32 * 32);
    int resized_h = std::max(32, (int)(height * scale + 0.5f) / 32 * 32);

    auto input_var = MNN::Express::_Input({1, 3, resized_h, resized_w}, MNN::Express::NCHW, halide_type_of<float>());
    auto input_ptr = input_var->writeMap<float>();

    // ImageProcess 预处理: RGBA → BGR + resize + normalize
    MNN::CV::ImageProcess::Config cfg;
    cfg.sourceFormat = MNN::CV::RGBA;
    cfg.destFormat   = MNN::CV::BGR;
    cfg.filterType   = MNN::CV::BILINEAR;
    float mean[3]  = {123.675f, 116.28f, 103.53f};
    float norm[3]  = {0.01712475f, 0.017507f, 0.01742919f};
    memcpy(cfg.mean,   mean, sizeof(mean));
    memcpy(cfg.normal, norm, sizeof(norm));

    auto pretreat = std::unique_ptr<MNN::CV::ImageProcess>(MNN::CV::ImageProcess::create(cfg));
    MNN::Matrix trans;
    trans.setScale((float)resized_w / width, (float)resized_h / height);
    pretreat->setMatrix(trans);

    std::vector<float> hwc_buf(resized_h * resized_w * 3);
    pretreat->convert(image_data, width, height, width * 4,
                      hwc_buf.data(), resized_w, resized_h, 3, 0, halide_type_of<float>());

    for (int c = 0; c < 3; c++)
        for (int h = 0; h < resized_h; h++)
            for (int w = 0; w < resized_w; w++)
                input_ptr[c * resized_h * resized_w + h * resized_w + w] = hwc_buf[(h * resized_w + w) * 3 + c];

    auto outputs = m_det_module->onForward({input_var});
    if (outputs.empty()) return results;

    auto out_var = outputs[0];
    auto out_info = out_var->getInfo();
    if (!out_info) return results;
    auto out_ptr = out_var->readMap<float>();

    int out_h = out_info->dim[2];
    int out_w = out_info->dim[3];
    const float det_thresh = 0.3f;

    // 二值化
    std::vector<uint8_t> bitmap(out_h * out_w, 0);
    for (int i = 0; i < out_h * out_w; i++)
        if (out_ptr[i] > det_thresh) bitmap[i] = 255;

    // 水平投影找文本行
    std::vector<std::pair<int,int>> text_rows;
    {
        std::vector<int> row_sum(out_h, 0);
        for (int r = 0; r < out_h; r++)
            for (int c = 0; c < out_w; c++)
                if (bitmap[r * out_w + c]) row_sum[r]++;

        bool in_text = false;
        int start = 0;
        for (int r = 0; r < out_h; r++) {
            if (!in_text && row_sum[r] > 3) {
                in_text = true; start = r;
            } else if (in_text && row_sum[r] <= 3) {
                in_text = false;
                if (r - start > 2) text_rows.emplace_back(start, r - 1);
            }
        }
        if (in_text && out_h - start > 2)
            text_rows.emplace_back(start, out_h - 1);
    }

    float scale_h = (float)height / out_h;
    float scale_w = (float)width  / out_w;

    for (auto& tr : text_rows) {
        int rs = tr.first, re = tr.second;
        int cs = out_w, ce = 0;
        for (int r = rs; r <= re; r++)
            for (int c = 0; c < out_w; c++)
                if (bitmap[r * out_w + c]) {
                    if (c < cs) cs = c;
                    if (c > ce) ce = c;
                }
        if (cs >= ce) continue;

        int margin = 2;
        rs = std::max(0, rs - margin);
        re = std::min(out_h - 1, re + margin);
        cs = std::max(0, cs - margin);
        ce = std::min(out_w - 1, ce + margin);

        TextBox tb;
        tb.confidence = 0.9f;
        tb.box_points[0] = cs * scale_w; tb.box_points[1] = rs * scale_h;
        tb.box_points[2] = ce * scale_w; tb.box_points[3] = rs * scale_h;
        tb.box_points[4] = ce * scale_w; tb.box_points[5] = re * scale_h;
        tb.box_points[6] = cs * scale_w; tb.box_points[7] = re * scale_h;
        results.push_back(tb);
    }
    return results;
}

// ============================================================
// 方向分类
// ============================================================
int OcrEngine::classify(const uint8_t* img, int w, int h, const float box[8], float& conf) {
    conf = 0.0f;
    if (!m_cls_module) return 0;

    // 裁剪到 48xH（分类模型：高48，宽可变）
    auto [crop, cw, ch] = crop_region(img, w, h, box, 0, 48);

    auto input_var = MNN::Express::_Input({1, 3, 48, cw}, MNN::Express::NCHW, halide_type_of<float>());
    auto ptr = input_var->writeMap<float>();

    for (int c = 0; c < 3; c++)
        for (int hh = 0; hh < 48; hh++)
            for (int ww = 0; ww < cw; ww++) {
                float val = (float)crop[(hh * cw + ww) * 3 + c];
                ptr[c * 48 * cw + hh * cw + ww] = (val / 255.0f - 0.5f) / 0.5f;
            }

    auto outputs = m_cls_module->onForward({input_var});
    if (outputs.empty() || !outputs[0]->getInfo()) return 0;

    auto out = outputs[0]->readMap<float>();
    auto info = outputs[0]->getInfo();
    int cls_num = info->dim[1];

    float sum = 0;
    for (int i = 0; i < cls_num; i++) sum += std::exp(out[i]);
    float p0 = std::exp(out[0]) / sum;  // 0 = 不翻转
    float p1 = std::exp(out[1]) / sum;  // 1 = 需翻转

    if (p1 > p0) { conf = p1; return 1; }
    conf = p0;
    return 0;
}

// ============================================================
// 裁剪区域（返回 RGB 数据）
// ============================================================
std::tuple<std::vector<uint8_t>, int, int> OcrEngine::crop_region(
    const uint8_t* src, int sw, int sh, const float box[8],
    int target_w, int target_h)
{
    int min_x = (int)std::max(0.0f, std::min({box[0], box[2], box[4], box[6]}));
    int min_y = (int)std::max(0.0f, std::min({box[1], box[3], box[5], box[7]}));
    int max_x = (int)std::min((float)sw - 1, std::max({box[0], box[2], box[4], box[6]}));
    int max_y = (int)std::min((float)sh - 1, std::max({box[1], box[3], box[5], box[7]}));

    int bw = max_x - min_x + 1;
    int bh = max_y - min_y + 1;
    if (bw < 2 || bh < 2) return {std::vector<uint8_t>(), 0, 0};

    int out_h, out_w;
    if (target_h > 0) {
        out_h = target_h;
        float h_avg = (point_distance(box[0], box[1], box[6], box[7]) +
                       point_distance(box[2], box[3], box[4], box[5])) / 2.0f;
        float w_avg = (point_distance(box[0], box[1], box[2], box[3]) +
                       point_distance(box[4], box[5], box[6], box[7])) / 2.0f;
        if (h_avg < 1 || w_avg < 1) return {std::vector<uint8_t>(), 0, 0};
        out_w = std::max(4, (int)(out_h * w_avg / h_avg + 0.5f));
    } else {
        out_h = bh;
        out_w = bw;
    }
    if (target_w > 0) out_w = target_w;

    std::vector<uint8_t> dst(out_h * out_w * 3, 0);
    for (int h = 0; h < out_h; h++) {
        for (int w = 0; w < out_w; w++) {
            float sy = min_y + (float)h / out_h * bh;
            float sx = min_x + (float)w / out_w * bw;
            int si = ((int)(sy + 0.5f) * sw + (int)(sx + 0.5f)) * 4;
            int di = (h * out_w + w) * 3;
            dst[di + 0] = src[si + 0];
            dst[di + 1] = src[si + 1];
            dst[di + 2] = src[si + 2];
        }
    }
    return {dst, out_w, out_h};
}

// ============================================================
// 识别单行文本
// ============================================================
TextLine OcrEngine::recognize_text(const uint8_t* img, int w, int h, const float box[8]) {
    TextLine result;
    memset(result.box, 0, sizeof(result.box));
    memcpy(result.box, box, sizeof(float) * 8);

    // 1. 方向分类
    float cls_conf = 0;
    int need_flip = classify(img, w, h, box, cls_conf);

    // 2. 裁剪到 32 高
    auto [crop_data, crop_w, crop_h] = crop_region(img, w, h, box, 0, 32);
    if (crop_data.empty()) return result;

    // 翻转
    if (need_flip && m_cls_module) {
        std::vector<uint8_t> flipped(crop_data.size());
        for (int hh = 0; hh < crop_h; hh++)
            for (int ww = 0; ww < crop_w; ww++)
                for (int cc = 0; cc < 3; cc++)
                    flipped[(hh * crop_w + ww) * 3 + cc] = crop_data[((crop_h - 1 - hh) * crop_w + ww) * 3 + cc];
        crop_data.swap(flipped);
    }

    int rec_h = 32;
    int rec_w = crop_w;

    auto input_var = MNN::Express::_Input({1, 3, rec_h, rec_w}, MNN::Express::NCHW, halide_type_of<float>());
    auto input_ptr = input_var->writeMap<float>();

    for (int c = 0; c < 3; c++)
        for (int hh = 0; hh < rec_h; hh++)
            for (int ww = 0; ww < rec_w; ww++) {
                float val = (float)crop_data[(hh * rec_w + ww) * 3 + c];
                input_ptr[c * rec_h * rec_w + hh * rec_w + ww] = (val / 255.0f - 0.5f) / 0.5f;
            }

    auto outputs = m_rec_module->onForward({input_var});
    if (outputs.empty() || !outputs[0]->getInfo()) return result;

    auto out_ptr = outputs[0]->readMap<float>();
    auto out_info = outputs[0]->getInfo();

    // 解析输出形状
    int seq_len = 0, class_num = 0;
    if (out_info->dim.size() == 4) {
        // NCHW: 1 x C x 1 x W  or  NHWC: 1 x 1 x W x C
        if (out_info->order == MNN::Express::NCHW) {
            class_num = out_info->dim[1];
            seq_len   = out_info->dim[3];
        } else {
            seq_len   = out_info->dim[2];
            class_num = out_info->dim[3];
        }
    } else if (out_info->dim.size() == 3) {
        seq_len   = out_info->dim[1];
        class_num = out_info->dim[2];
    }
    if (seq_len <= 0 || class_num <= 0) return result;

    // 重排为 (seq_len, class_num)
    std::vector<float> reshaped(seq_len * class_num);
    if (out_info->order == MNN::Express::NCHW && out_info->dim.size() == 4 && out_info->dim[2] == 1) {
        for (int c = 0; c < class_num; c++)
            for (int t = 0; t < seq_len; t++)
                reshaped[t * class_num + c] = out_ptr[c * seq_len + t];
    } else {
        memcpy(reshaped.data(), out_ptr, seq_len * class_num * sizeof(float));
    }

    float confidence = 0;
    result.text = decode(reshaped.data(), seq_len, class_num, confidence);
    result.confidence = confidence;
    return result;
}

// ============================================================
// 完整识别流程
// ============================================================
std::vector<TextLine> OcrEngine::recognize(const uint8_t* image_data, int width, int height) {
    auto boxes = detect(image_data, width, height);
    std::sort(boxes.begin(), boxes.end(),
              [](const TextBox& a, const TextBox& b) { return a.box_points[1] < b.box_points[1]; });

    std::vector<TextLine> results;
    for (auto& box : boxes) {
        auto line = recognize_text(image_data, width, height, box.box_points);
        if (!line.text.empty()) results.push_back(line);
    }
    return results;
}

// ============================================================
// 全局单例
// ============================================================
static OcrEngine* g_engine = nullptr;

int global_init(int num_thread) {
    if (g_engine) {
        MNN_PRINT("[OCR] Engine already initialized\n");
        return 0;
    }
    auto* eng = new OcrEngine();
    if (!eng->init(num_thread)) {
        delete eng;
        g_engine = nullptr;
        return -1;
    }
    g_engine = eng;
    return 0;
}

void global_destroy() {
    delete g_engine;
    g_engine = nullptr;
}

OcrEngine* global_instance() {
    return g_engine;
}

} // namespace OCR
} // namespace MNN
