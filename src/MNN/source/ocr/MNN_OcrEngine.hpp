//
//  MNN_OcrEngine.hpp
//  MNN
//
//  PP-OCRv5 引擎内部实现头文件
//

#ifndef MNN_OcrEngine_hpp
#define MNN_OcrEngine_hpp

#include <vector>
#include <string>
#include <memory>
#include <tuple>

#include <MNN/Interpreter.hpp>
#include <MNN/expr/Expr.hpp>
#include <MNN/expr/ExprCreator.hpp>
#include <MNN/expr/Module.hpp>
#include <MNN/expr/Executor.hpp>
#include <MNN/ImageProcess.hpp>
#include <MNN/Matrix.h>
#include <MNN/MNNDefine.h>

#include "ocr/resource.h"

namespace MNN {
namespace OCR {

// 文本框
struct TextBox {
    float box_points[8];
    float confidence;
};

// 单行识别结果
struct TextLine {
    std::string text;
    float confidence;
    float box[8];
};

// OCR 引擎
class OcrEngine {
public:
    OcrEngine();
    ~OcrEngine();

    bool init(int num_thread);
    std::vector<TextLine> recognize(const uint8_t* image_data, int width, int height);

private:
    std::vector<TextBox> detect(const uint8_t* image_data, int width, int height);
    int classify(const uint8_t* img, int w, int h, const float box[8], float& conf);
    TextLine recognize_text(const uint8_t* img, int w, int h, const float box[8]);

    std::tuple<std::vector<uint8_t>, int, int> crop_region(
        const uint8_t* src, int sw, int sh, const float box[8],
        int target_w, int target_h);

    bool load_dict_from_memory(const std::vector<uint8_t>& data);
    std::string decode(const float* data, int seq_len, int class_num, float& confidence);

    std::shared_ptr<MNN::Express::Module> m_det_module;
    std::shared_ptr<MNN::Express::Module> m_rec_module;
    std::shared_ptr<MNN::Express::Module> m_cls_module;
    std::vector<std::string> m_dict;
    int m_num_thread = 4;
};

// 全局单例管理函数（内部使用）
int  global_init(int num_thread);
void global_destroy();
OcrEngine* global_instance();

} // namespace OCR
} // namespace MNN

#endif /* MNN_OcrEngine_hpp */
