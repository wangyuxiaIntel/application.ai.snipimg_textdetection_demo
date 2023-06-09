#pragma once

#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>


namespace PaddleOCR {

cv::Mat GetRotateCropImage(const cv::Mat &srcimage,std::vector<std::vector<int>> box);

enum RESIZE_MODE {
    RESIZE_FILL,
    RESIZE_KEEP_ASPECT,
    RESIZE_KEEP_ASPECT_LETTERBOX
};

cv::Mat resizeImageExt(const cv::Mat& mat, int width, int height, RESIZE_MODE resizeMode = RESIZE_FILL,
                       cv::InterpolationFlags interpolationMode = cv::INTER_LINEAR, cv::Rect* roi = nullptr,
                       cv::Scalar BorderConstant = cv::Scalar(0, 0, 0));

void normalize(cv::Mat *im, const std::vector<float> &mean, const std::vector<float> &scale, const bool is_scale);




struct OCRPredictResult {
  std::vector<std::vector<int>> box;
  std::string text;
  float score = -1.0;
  float cls_score;
  int cls_label = -1;
};

void sorted_boxes(std::vector<OCRPredictResult>& ocr_result);

void print_result(const std::vector<OCRPredictResult> &ocr_result);
void VisualizeBboxes(const cv::Mat &srcimg, const std::vector<OCRPredictResult> &ocr_result, const std::string &save_path);

}//namespace PaddleOCR