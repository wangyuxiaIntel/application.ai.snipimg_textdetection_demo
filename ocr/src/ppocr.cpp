#include "ppocr.hpp"


namespace PaddleOCR {

// WYX TODO map list config
PPOCR::PPOCR(ov::Core& core, const std::string& det_modelPath, const std::string& det_modelType, const std::string& det_deviceName,
                 bool det_is_dynamic_model, bool det_use_ppp_auto_resize,
                 const std::string& rec_modelPath, const std::string& rec_modelType, const std::string& rec_deviceName,
                 bool rec_is_dynamic_model, bool rec_use_ppp_auto_resize, const std::string& rec_dict) {
  //WYX TODO verify path effectiveness
  this->detector_ = new DBDetector(core, det_modelPath, det_modelType,  det_deviceName, det_is_dynamic_model, det_use_ppp_auto_resize);

  this->recognizer_ = new CRNNRecognizer(core, rec_modelPath, rec_modelType,  rec_deviceName, rec_dict, rec_is_dynamic_model, rec_use_ppp_auto_resize);


//   if (FLAGS_rec) {
//     this->recognizer_ = new CRNNRecognizer(
//         FLAGS_rec_model_dir, FLAGS_use_gpu, FLAGS_gpu_id, FLAGS_gpu_mem,
//         FLAGS_cpu_threads, FLAGS_enable_mkldnn, FLAGS_rec_char_dict_path,
//         FLAGS_use_tensorrt, FLAGS_precision, FLAGS_rec_batch_num,
//         FLAGS_rec_img_h, FLAGS_rec_img_w);
//   }

};


std::vector<OCRPredictResult> PPOCR::ocr(cv::Mat img) {
  this->detector_->m_preprocess_time_elapsed=0.0;
  this->detector_->m_inference_time_elapsed=0.0;
  this->detector_->m_postprocess_time_elapsed=0.0;
  this->recognizer_->m_preprocess_time_elapsed=0.0;
  this->recognizer_->m_inference_time_elapsed=0.0;
  this->recognizer_->m_postprocess_time_elapsed=0.0;
  std::vector<OCRPredictResult> ocr_result;
  // det
  this->det(img, ocr_result);
  
  // crop image
   auto add_preprocess_start = std::chrono::steady_clock::now();
  std::vector<cv::Mat> text_img_list;
  for (int j = 0; j < ocr_result.size(); j++) {
      cv::Mat crop_img;
      crop_img = GetRotateCropImage(img, ocr_result[j].box);
      text_img_list.push_back(crop_img);
  }
  //cv::imwrite("D:/OpenVINO_Model_Zoo/ppocr_resize_crop_img.jpg", text_img_list[0]);

  auto add_preprocess_end = std::chrono::steady_clock::now();
  std::chrono::duration<float> add_preprocess_diff = add_preprocess_end - add_preprocess_start;
  this->recognizer_->m_preprocess_time_elapsed +=double(add_preprocess_diff.count() * 1000);

  this->rec(text_img_list, ocr_result);

  slog::info<<"   ---Timer---    Det Pre: "<< this->detector_->m_preprocess_time_elapsed <<slog::endl;
  slog::info<<"   ---Timer---    Det Infer: "<< this->detector_->m_inference_time_elapsed <<slog::endl;
  slog::info<<"   ---Timer---    Det Post: "<< this->detector_->m_postprocess_time_elapsed <<slog::endl;
  slog::info<<"   ---Timer---    Rec Pre: "<< this->recognizer_->m_preprocess_time_elapsed <<slog::endl;
  slog::info<<"   ---Timer---    Rec Infer: "<< this->recognizer_->m_inference_time_elapsed <<slog::endl;
  slog::info<<"   ---Timer---    Rec Post: "<< this->recognizer_->m_postprocess_time_elapsed <<slog::endl;

  return ocr_result;
}

void PPOCR::det(cv::Mat img, std::vector<OCRPredictResult> &ocr_results) {
  
  std::vector<std::vector<std::vector<int>>> boxes;
  this->detector_->Infer(img, boxes);

  for (int i = 0; i < boxes.size(); i++) {
    OCRPredictResult res;
    res.box = boxes[i];
    ocr_results.push_back(res);
  }

  // sort boex from top to bottom, from left to right
  auto add_postprocess_start = std::chrono::steady_clock::now();
  sorted_boxes(ocr_results);

  auto add_postprocess_end = std::chrono::steady_clock::now();
  std::chrono::duration<float> add_postprocess_diff = add_postprocess_end - add_postprocess_start;
  this->detector_->m_postprocess_time_elapsed +=double(add_postprocess_diff.count() * 1000);
}


void PPOCR::rec(std::vector<cv::Mat> text_img_list, std::vector<OCRPredictResult> &ocr_results){
  std::vector<std::string> rec_texts(text_img_list.size(), "");
  std::vector<float> rec_text_scores(text_img_list.size(), 0);
  int texti=0;
  for(auto text_img:text_img_list){
    this->recognizer_->Infer(text_img,rec_texts,rec_text_scores, texti);
    // texti+=1;
  }

  for (int i = 0; i < rec_texts.size(); i++) {
    ocr_results[i].text = rec_texts[i];
    ocr_results[i].score = rec_text_scores[i];
  }


}




PPOCR::~PPOCR() {
  if (this->detector_ != nullptr) {
    delete this->detector_;
  }
  if (this->recognizer_ != nullptr) {
    delete this->recognizer_;
  }
};

} // namespace PaddleOCR