#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>

#include "postprocess.hpp"
#include "common.hpp"
#include "slog.hpp"
#include "ocv_common.hpp"
#include "utility.hpp"


namespace PaddleOCR{

class DBDetector{
public:
    DBDetector(ov::Core& core, const std::string& modelPath, const std::string& modelType, const std::string& deviceName,
                bool is_dynamic_model=false, bool use_ppp_auto_rezie = false);

    ov::Tensor PreprocessInputImg(cv::Mat img,float& ratio_h, float& ratio_w);
    void Infer(const cv::Mat img, std::vector<std::vector<std::vector<int>>> &boxes);
    ~DBDetector() = default;
    
//pre-process
//post-process

protected:
    const std::string m_modelPath;
    const std::string m_deviceName;
    const std::string m_modelType;

    ov::Core m_core;
    ov::InferRequest m_infer_request;
    std::shared_ptr<ov::Model> m_model;
    
    //model attribute
    std::vector<std::string> m_model_input_names;
    std::vector<std::string> m_model_output_names;
    ov::Shape m_model_input_shape_static;
    ov::PartialShape m_model_input_shape_partial;
    ov::Layout m_model_input_layout;
    //ov::Layout m_model_output_layout={"NCHW"}; //WYX TODO why illegal
    ov::Layout m_model_output_layout;
    cv::Size m_model_input_size;

    //pre process parameter
    bool m_use_ppp_auto_resize;
    std::vector<float> m_mean_ = {0.485f, 0.456f, 0.406f};
    std::vector<float> m_scale_ = {1 / 0.229f, 1 / 0.224f, 1 / 0.225f};
    bool m_is_scale_ = true;

    //input image limit
    std::string m_limit_type = "max";
    int m_limit_side_len = 960;

    bool m_is_dynamic_model= false;

public:
    //Timer
    double m_preprocess_time_elapsed=0.0;
    double m_inference_time_elapsed=0.0;
    double m_postprocess_time_elapsed=0.0;

};

} //namespace PaddleOCR