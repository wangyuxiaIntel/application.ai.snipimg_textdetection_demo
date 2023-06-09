
#pragma once
#include <string>
#include "ppocr_det.hpp"
#include "ppocr_rec.hpp"

namespace PaddleOCR {

class PPOCR {
public:
  explicit PPOCR(ov::Core& core, const std::string& det_modelPath, const std::string& det_modelType, const std::string& det_deviceName,
                 bool det_is_dynamic_model, bool det_use_ppp_auto_resize,
                 const std::string& rec_modelPath, const std::string& rec_modelType, const std::string& rec_deviceName,
                 bool rec_is_dynamic_model, bool rec_use_ppp_auto_resize, const std::string& rec_dict);
  ~PPOCR();

  std::vector<OCRPredictResult> ocr(cv::Mat img);

protected:
    void det(cv::Mat img, std::vector<OCRPredictResult> &ocr_results);
    void rec(std::vector<cv::Mat> text_img_list, std::vector<OCRPredictResult> &ocr_results);


private:
  DBDetector *detector_ = nullptr;
  CRNNRecognizer *recognizer_ = nullptr;
};

} // namespace PaddleOCR