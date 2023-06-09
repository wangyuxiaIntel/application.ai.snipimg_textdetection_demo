#include "ppocr_det.hpp"

namespace PaddleOCR{

DBDetector::DBDetector(ov::Core& core, const std::string& modelPath, const std::string& modelType, const std::string& deviceName,
                        bool is_dynamic_model, bool use_ppp_auto_resize) :
    m_modelPath(modelPath), m_modelType(modelType), m_deviceName(deviceName),
    m_core(core), m_is_dynamic_model(is_dynamic_model), m_use_ppp_auto_resize(use_ppp_auto_resize)
{
    slog::info << "\n \n ---------------------------- Reading Detection model ----------------------------" << slog::endl;
    slog::info << m_modelPath << slog::endl;
    m_model = m_core.read_model(m_modelPath);

    logBasicModelInfo(m_model);
    // Collect input names
    ov::OutputVector model_inputs=m_model->inputs();
    if (model_inputs.size() != 1) {
        throw std::runtime_error("Det model should have only one input");
    }
    for(const ov::Output<ov::Node>& input:model_inputs)
    {
        m_model_input_names.push_back(input.get_any_name());
        // slog::info<<"Det model Input " << input.get_any_name() <<":  " <<input.get_partial_shape()<<slog::endl;
    }

    if(m_is_dynamic_model){
        m_model_input_shape_partial = m_model->input().get_partial_shape();
    } else{
        m_model_input_shape_static = m_model->input().get_shape();
    }

    if ((m_is_dynamic_model&&m_model_input_shape_partial.size() != 4)||((!m_is_dynamic_model)&&m_model_input_shape_static.size()!=4)) {
        throw std::runtime_error("The model should have 4-dimensional input");
    }
    
    m_model_input_layout = ov::layout::get_layout(m_model->input());
    if (m_model_input_layout.empty()) {
        // prev release model has NCHW layout but it was not specified at IR
        m_model_input_layout = { "NCHW" };
    }

    // Collect output names
    ov::OutputVector outputs = m_model->outputs();
    for (const ov::Output<ov::Node>& output : outputs) {
        m_model_output_names.push_back(output.get_any_name());
    }
    
    // Configuring input and output
    ov::preprocess::PrePostProcessor ppp(m_model);
    ppp.input().tensor()
        .set_element_type(ov::element::f32)
        .set_layout({ "NHWC" });

    if (!m_is_dynamic_model&&m_use_ppp_auto_resize) {
        ppp.input().tensor()
            .set_spatial_dynamic_shape();

        ppp.input().preprocess()
            .convert_element_type(ov::element::f32)
            .resize(ov::preprocess::ResizeAlgorithm::RESIZE_LINEAR);
    }
    ppp.input().model().set_layout(m_model_input_layout);

    m_model = ppp.build();
    slog::info << "Preprocessor configuration: " << slog::endl;
    slog::info << ppp << slog::endl;

    // Loading model to the device
    ov::CompiledModel compiled_model = m_core.compile_model(m_model, m_deviceName);
    logCompiledModelInfo(compiled_model, m_modelPath, m_deviceName, m_modelType);

    // Creating infer request
    m_infer_request = compiled_model.create_infer_request();

}


//normalization. resize input img. use ppp reconduct model.
ov::Tensor DBDetector::PreprocessInputImg(cv::Mat img,float& ratio_h, float& ratio_w)
{
    //resize
    const int img_H=img.rows;
    const int img_W=img.cols;
    int resize_h=img_H;
    int resize_w=img_W;
    cv::Mat resize_img;
    
    if(!m_is_dynamic_model){
        const int model_input_W=m_model_input_shape_static[ov::layout::width_idx(m_model_input_layout)];
        const int model_input_H=m_model_input_shape_static[ov::layout::height_idx(m_model_input_layout)];
        if(model_input_W%32!=0||model_input_H%32!=0||std::max(model_input_W,model_input_H)>m_limit_side_len){
            //WYX TODO throw error
            slog::info << "---Error--- Input size of static DB model should be mod of 32 !! " << slog::endl;
            slog::info << "---Error--- Or max(H,W) shoudl < 960  !! " << slog::endl;
        }
        resize_h = model_input_H;
        resize_w = model_input_W;
        //when model is static, don't need to round 32; because model must 32x.        
    }else{
        if(std::max(img_H,img_W)>960){
            if(img_H>img_W){
                resize_h=960;
                resize_w=(int)((960.0/img_H)*img_W);
            }else{
                resize_w=960;
                resize_h=(int)((960.0/img_W)*img_H);
            }
        }

        resize_h=std::max(int(resize_h/32)*32,32);
        resize_w=std::max(int(resize_w/32)*32,32);
    }
    ratio_h=float(resize_h)/float(img_H);
    ratio_w=float(resize_w)/float(img_W);
    if(!m_use_ppp_auto_resize){
        // dynamic model and use ppp resize shoudn't exit at the same time.
        //resize input img to model input tensor size
        resize_img = resizeImageExt(img, resize_w, resize_h);
    }else{
        resize_img=img;
    }

    //normalization
    normalize(&resize_img, m_mean_, m_scale_, m_is_scale_);

    //wrap img to tensor
    ov::Tensor input_img_tensor=wrapMat2Tensor(resize_img);
    
    return input_img_tensor;
}

void DBDetector::Infer(const cv::Mat img, std::vector<std::vector<std::vector<int>>> &boxes)
{
    float ratio_h=1.0;
    float ratio_w=1.0;
    auto preprocess_start = std::chrono::steady_clock::now();
    ov::Tensor input_img_tensor=PreprocessInputImg(img,ratio_h,ratio_w);
    auto preprocess_end = std::chrono::steady_clock::now();
    
    auto inference_start = std::chrono::steady_clock::now();
    m_infer_request.set_input_tensor(input_img_tensor);
    m_infer_request.infer();
    auto inference_end = std::chrono::steady_clock::now();


    auto postprocess_start = std::chrono::steady_clock::now();
    ov::runtime::Tensor output_tensor=m_infer_request.get_output_tensor();
    ov::Shape output_shape=output_tensor.get_shape();
    slog::info<<"Det model Output Shape:  " << output_shape <<slog::endl;
    m_model_output_layout={"NCHW"};
    //WYX TODO why error
    // const size_t output_H = output_shape[ov::layout::height_idx(m_model_output_layout)];
    // const size_t output_W = output_shape[ov::layout::width_idx(m_model_output_layout)];
    size_t output_H = output_shape[ov::layout::height_idx(m_model_output_layout)];
    size_t output_W = output_shape[ov::layout::width_idx(m_model_output_layout)];
    const size_t output_size=std::accumulate(output_shape.begin(),  output_shape.end(), 1 , std::multiplies<int>());//calculate the multipy of det_output_shape

    float* p_out_tensor =  output_tensor.data<float>();
    std::vector<float>  vec_output_tensor(p_out_tensor, p_out_tensor + output_size);

    // //Imwrite output_mask
    // cv::Mat img_output_mask = cv::Mat::zeros(output_H,output_W,CV_32FC1);
    // for(int h=0;h<output_H;h++){
    //     for(int w=0;w<output_W;w++){
    //         img_output_mask.at<float>(h,w)=vec_output_tensor[h*output_W+w]*255;
    //     }
    // }
    // cv::imwrite("D:/OpenVINO_Model_Zoo/ppocr_result_mask.jpg", img_output_mask);

    //DB post_process
    const int n = output_H * output_W;
    std::vector<float> pred(n, 0.0);
    std::vector<unsigned char> cbuf(n, ' ');
    for (int i = 0; i < n; i++) 
    {
        pred[i] = float(vec_output_tensor[i]);
        cbuf[i] = (unsigned char)((vec_output_tensor[i]) * 255);
    }
    cv::Mat cbuf_map(output_H, output_W, CV_8UC1, (unsigned char *)cbuf.data());
    cv::Mat pred_map(output_H, output_W, CV_32F, (float *)pred.data());
    const double threshold = 0.3 * 255;
    const double maxvalue = 255;
    cv::Mat bit_map;
    cv::threshold(cbuf_map, bit_map, threshold, maxvalue, cv::THRESH_BINARY);


    std::vector<double> det_times;
    DBPostProcessor det_post_processor;

    boxes = det_post_processor.BoxesFromBitmap(pred_map, bit_map, 0.5, 2.0,"slow");
    boxes = det_post_processor.FilterTagDetRes(boxes, ratio_h, ratio_w, img);
    auto postprocess_end = std::chrono::steady_clock::now();
    
    std::chrono::duration<float> preprocess_diff = preprocess_end - preprocess_start;
    m_preprocess_time_elapsed += double(preprocess_diff.count() * 1000);
    std::chrono::duration<float> inference_diff = inference_end - inference_start;
    m_inference_time_elapsed += double(inference_diff.count() * 1000);
    std::chrono::duration<float> postprocess_diff = postprocess_end - postprocess_start;
    m_postprocess_time_elapsed += double(postprocess_diff.count() * 1000);
}


} //namespace PaddleOCR