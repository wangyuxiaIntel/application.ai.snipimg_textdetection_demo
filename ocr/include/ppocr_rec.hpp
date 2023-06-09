#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>
#include <fstream>

#include "slog.hpp"
#include "utility.hpp"
#include "ocv_common.hpp"


namespace PaddleOCR{

class CRNNRecognizer{
public:
    CRNNRecognizer(ov::Core& core, const std::string& modelPath, const std::string& modelType, const std::string& deviceName,
                const std::string& rec_dict_path, bool is_dynamic_model=false, bool use_ppp_auto_rezie = false);

    ov::Tensor PreprocessInputImg(cv::Mat img);
    void Infer(const cv::Mat img, std::vector<std::string>& rec_texts, std::vector<float>& rec_text_scores, int& test_index);
    ~CRNNRecognizer() = default;
    
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
    std::vector<float> m_mean_ = {0.5f, 0.5f, 0.5f};
    std::vector<float> m_scale_ = {1 / 0.5f, 1 / 0.5f, 1 / 0.5f};
    bool m_is_scale_ = true;

    //input image limit
    std::string m_limit_type = "h fixed";
    int m_reg_img_h = 48;

    bool m_is_dynamic_model= false;

    std::vector<std::string> m_label_list_;
public:
    //Timer
    double m_preprocess_time_elapsed=0.0;
    double m_inference_time_elapsed=0.0;
    double m_postprocess_time_elapsed=0.0;

};

} //namespace PaddleOCR