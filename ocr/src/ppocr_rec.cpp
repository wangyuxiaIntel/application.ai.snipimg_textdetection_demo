#include "ppocr_rec.hpp"

namespace PaddleOCR{

template <class ForwardIterator>
size_t argmax_index(ForwardIterator first, ForwardIterator last) 
{
    return std::distance(first, std::max_element(first, last));
}

std::vector<std::string> ReadLabelDict(const std::string &path) {
  std::ifstream in(path);
  std::string line;
  std::vector<std::string> m_vec;
  if (in) {
    while (getline(in, line)) {
      m_vec.push_back(line);
    }
  } else {
    std::cout << "no such label file: " << path << ", exit the program..."
              << std::endl;
    exit(1);
  }
  return m_vec;
}



CRNNRecognizer::CRNNRecognizer(ov::Core& core, const std::string& modelPath, const std::string& modelType, const std::string& deviceName,
                        const std::string& rec_dict_path, bool is_dynamic_model, bool use_ppp_auto_resize) :
    m_modelPath(modelPath), m_modelType(modelType), m_deviceName(deviceName),
    m_core(core), m_is_dynamic_model(is_dynamic_model), m_use_ppp_auto_resize(use_ppp_auto_resize)
{
    slog::info << "\n \n ---------------------------- Reading Recognition model ----------------------------" << slog::endl;
    slog::info << m_modelPath << slog::endl;
    m_model = m_core.read_model(m_modelPath);
    logBasicModelInfo(m_model);
    // Collect input names
    ov::OutputVector model_inputs=m_model->inputs();
    if (model_inputs.size() != 1) {
        throw std::runtime_error("Recognition model should have only one input");
    }
    for(const ov::Output<ov::Node>& input:model_inputs)
    {
        m_model_input_names.push_back(input.get_any_name());
        // slog::info<<"Rec model Input " << input.get_any_name() <<":  " <<input.get_partial_shape()<<slog::endl;
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

    m_label_list_ = ReadLabelDict(rec_dict_path);
    m_label_list_.insert(m_label_list_.begin(),"#");
    m_label_list_.push_back(" ");


}

ov::Tensor CRNNRecognizer::PreprocessInputImg(cv::Mat img){
    //resize
    const int img_H=img.rows;
    const int img_W=img.cols;
    int resize_h=m_reg_img_h;
    int resize_w=int(float(img_W)/img_H)*m_reg_img_h;

    cv::Mat resize_img;
    resize_img = resizeImageExt(img, resize_w, resize_h);

    //normalization
    normalize(&resize_img, m_mean_, m_scale_, m_is_scale_);

    //wrap img to tensor
    ov::Tensor input_img_tensor=wrapMat2Tensor(resize_img);
    
    return input_img_tensor;
}


void CRNNRecognizer::Infer(const cv::Mat img, std::vector<std::string>& rec_texts, std::vector<float>& rec_text_scores, int& text_index)
{
    // slog::info<<"Rec model Strat:  " <<slog::endl;
    auto preprocess_start = std::chrono::steady_clock::now();
    ov::Tensor input_img_tensor=PreprocessInputImg(img);
    auto preprocess_end = std::chrono::steady_clock::now();
    // slog::info<<"Rec model Preprocess end:  " <<slog::endl;

    auto inference_start = std::chrono::steady_clock::now();
    m_infer_request.set_input_tensor(input_img_tensor);
    m_infer_request.infer();
    auto inference_end = std::chrono::steady_clock::now();

    //----------------------parse rec_output_tensor
    auto postprocess_start = std::chrono::steady_clock::now();
    ov::Tensor output_tensor = m_infer_request.get_output_tensor();
    auto p_output_tensor =output_tensor.data<float>();

    auto output_shape = output_tensor.get_shape();
    // slog::info<<"Rec model Output:  " << output_shape <<slog::endl;
    int output_size = std::accumulate(output_shape.begin(), output_shape.end(), 1, std::multiplies<int>());
    std::vector<float> vec_output_tensor(p_output_tensor,p_output_tensor+output_size);

    for (size_t n = 0; n<output_shape[0]; n++)
    {
        std::string str_res;
        int argmax_idx;
        int last_index=0;
        float score=0.f;
        int count=0;
        float max_value=0.0f;

        for(int ts=0;ts<output_shape[1];ts++)
        {
            //get_index
            argmax_idx = argmax_index(&vec_output_tensor[(n*output_shape[1]+ts)*output_shape[2]],
                                        &vec_output_tensor[(n*output_shape[1]+ts+1)*output_shape[2]]);  
             // get score
            max_value = float(*std::max_element(
                            &vec_output_tensor[(n * vec_output_tensor[1] + ts) * vec_output_tensor[2]],
                            &vec_output_tensor[(n * vec_output_tensor[1] + ts + 1) * vec_output_tensor[2]]));
            if(argmax_idx>0&&(!(ts>0&&argmax_idx==last_index))){
                score += max_value;
                count += 1;
                str_res += m_label_list_[argmax_idx];
            }
            last_index=argmax_idx;
        }
        if (count==0){
            std::cout<<"Unrecognized character in batch"<< n <<" !!"<<std::endl;
            continue;
        }
        score /=count;
        // std::cout<<"Recognize string: "<< str_res<< " , score: " << score <<" , character count:  "<<count<<std::endl;
        rec_texts[text_index] = str_res;
        rec_text_scores[text_index] = score;
        text_index+=1;
    }
    auto postprocess_end = std::chrono::steady_clock::now();
    
    std::chrono::duration<float> preprocess_diff = preprocess_end - preprocess_start;
    m_preprocess_time_elapsed +=double(preprocess_diff.count() * 1000);
    std::chrono::duration<float> inference_diff = inference_end - inference_start;
    m_inference_time_elapsed +=double(inference_diff.count() * 1000);
    std::chrono::duration<float> postprocess_diff = postprocess_end - postprocess_start;
    m_postprocess_time_elapsed +=double(postprocess_diff.count() * 1000);

}




}//namespace PaddleOCR