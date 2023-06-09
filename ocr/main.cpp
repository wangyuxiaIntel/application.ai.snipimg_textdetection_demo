#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>


#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>


#include "common.hpp"

#include "ppocr.hpp"
#include "utility.hpp"
#include "snipimg_text_detection_demo.hpp"


#include "snip_gui.hpp"
#include "clipboard.hpp"

namespace SnipGUI{
    HINSTANCE g_hInstance = NULL;
    HWND g_SnipHwnd;
    cv::Mat g_snip_img;
    bool g_wait_for_snip=true;


} //namespace SnipGUI


// FILE* ocrResFILE=NULL;
static char* ResTmpDir="./temp/ocr_result.txt";
bool Alt_DOWN=false;
bool wait_ocr=true;


ov::Core core;
PaddleOCR::PPOCR* ocr=nullptr;


bool isFileExists_stat(std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

bool ParseAndCheckCommandLine(int argc, char* argv[]) {
    // Parsing and validating input arguments
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);

    if (FLAGS_h) {
        showUsage();
        showAvailableDevices();
        return false;
    }
    if(!isFileExists_stat(FLAGS_m_det)){
        std::cout<<"The det model file is not exit: "<<FLAGS_m_det<<std::endl;
        return false;
    }
    if(!isFileExists_stat(FLAGS_m_rec)){
        std::cout<<"The rec model file is not exit: "<<FLAGS_m_rec<<std::endl;
        return false;
    }
    //WYX TODO  verify params
    return true;
}

void save_result_to_file(std::vector<PaddleOCR::OCRPredictResult> ocr_result, std::string dir){
    std::ofstream f_res;
    f_res.open(dir,std::ios::app);
    for (int i = 0; i < ocr_result.size(); i++) {
        if (ocr_result[i].score != -1.0)
        {
            f_res << ocr_result[i].text << " \n";
        }
    }
    f_res << "\n \n";
    f_res.close();
}

void launchNotepad(const char* file) {
	char cmd[256] = { 0 };
	snprintf(cmd, 255, "notepad %s", file);
	STARTUPINFOA sinf = { 0 };
	sinf.dwFlags |= STARTF_USESHOWWINDOW;
	sinf.wShowWindow |= SW_SHOWNORMAL;
	PROCESS_INFORMATION pinf = { 0 };

	sinf.dwFlags = STARTF_USESTDHANDLES;
	sinf.cb = sizeof(STARTUPINFOA);
	if (!CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, TRUE, 0, NULL, NULL, &sinf, &pinf)) {
		printf("Create Process Failed\n");
	}
	CloseHandle(pinf.hProcess);
	CloseHandle(pinf.hThread);
}

void keyboardhookfunc(PaddleOCR::PPOCR* ocr){
    cv::Mat clipimg=imread_clipboard();
    if(!clipimg.data){
        std::cout<<"Clipboard img is empty !!!"<<std::endl;
        return;
    }
    cv::Mat infer_img;
    cv::cvtColor(clipimg, infer_img, cv::COLOR_RGBA2RGB);
    cv::imwrite("./temp/ocr_clip_img.jpg", infer_img);
    std::cout<<"\n \n ---------------------------------- Infer ClipBoard Image ----------------------------------------"<<std::endl;
    if(!infer_img.data){
        std::cout << "---Error--- Can't transform clip img !!! "<< std::endl;
    }
    std::cout <<" Clip img size,  H : "<<infer_img.rows<<",   W : "<< infer_img.cols<<",   C: "<< infer_img.channels()<<std::endl;
    std::vector<PaddleOCR::OCRPredictResult> ocr_result = ocr->ocr(infer_img);
    print_result(ocr_result);
    save_result_to_file(ocr_result,ResTmpDir);
    VisualizeBboxes(infer_img, ocr_result, "./temp/ocr_clip_img_result.jpg");
    return;
}

static LRESULT keyboardLLHookCallback(int code, WPARAM wParam, LPARAM lParam) {
	if (code < 0)
	{
        return CallNextHookEx(0, code, wParam, lParam);
	}
	KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
    // std::cout<<"vkCode:      "<<kb->vkCode<<std::endl;
    if(!Alt_DOWN && (kb->vkCode == VK_MENU || kb->vkCode == VK_LMENU ||kb->vkCode == VK_RMENU) ){
        Alt_DOWN=true;
        // std::cout<<"alt down:      "<<kb->vkCode<<std::endl;
    }
	if(wait_ocr&&wParam==WM_KEYDOWN){
        // std::cout<<"WM Keydown:      "<<kb->vkCode<<std::endl;
        if(Alt_DOWN && kb->vkCode=='T'){
            Alt_DOWN=false;
            wait_ocr=false;
            // std::cout<<"T down:      "<<kb->vkCode<<std::endl;
        }
    }
    if(!wait_ocr){
        keyboardhookfunc(ocr);
        wait_ocr=true;
    }

	return CallNextHookEx(0, code, wParam, lParam);
}

int main(int argc, char* argv[]){
    if (!ParseAndCheckCommandLine(argc, argv)) {
        return 0;
    }
    if(!remove(ResTmpDir)==0) {
        std::cout<<"Failed to remove the last result temp txt file: "<< ResTmpDir <<std::endl;
        return 0;
    }

    //create windows
    HWND hConWnd = GetConsoleWindow();
    SnipGUI::g_hInstance = (HINSTANCE)GetWindowLong(hConWnd, GWLP_HINSTANCE);

    //Prepare for model inference
    // ov::Core core;
    ocr =  new PaddleOCR::PPOCR(core, FLAGS_m_det,"DBnet Detection" , FLAGS_d_det, FLAGS_dymc_det, FLAGS_upr_det,
                                                FLAGS_m_rec,"CRNN Recognition" , FLAGS_d_rec, FLAGS_dymc_rec, FLAGS_upr_rec,FLAGS_rec_dict);

    MSG msg = { };
    std::cout<<"---------------------------------- Start Application ----------------------------------------"<<std::endl;
    std::cout<<"---------------------------------- Click Button to snip an image ----------------------------------------"<<std::endl;


    if(FLAGS_is=="clip"){
        HHOOK hk1 = SetWindowsHookExA(WH_KEYBOARD_LL, keyboardLLHookCallback, 0, 0);

        while (GetMessage(&msg, NULL, 0, 0))
        {
            // printf("start send massage");
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        UnhookWindowsHookEx(hk1);
    }
    else{
        SnipGUI::SnipWindow* pSnipWindow = nullptr;
        SnipGUI::SnipWindow::Create("Snip Zone",&pSnipWindow); 
        SnipGUI::g_SnipHwnd=pSnipWindow->m_hwnd;
    
        POINT pt = { 150, 20 };
        SIZE size = { 500, 100 };
        SnipGUI::MainWindow* pMainWindow = nullptr;
        SnipGUI::MainWindow::Create("MainWindow",size,pt,&pMainWindow);

        while (GetMessage(&msg, NULL, 0, 0))
        {
            // printf("start send massage");
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if(SnipGUI::g_wait_for_snip==false)
            {
                SnipGUI::g_wait_for_snip=true;
                cv::Mat infer_img;
                cv::cvtColor(SnipGUI::g_snip_img, infer_img, cv::COLOR_RGBA2RGB);
                cv::imwrite("./temp/ocr_snip_img.jpg", infer_img);

                std::cout<<"\n \n ---------------------------------- Infer Snip Image ----------------------------------------"<<std::endl;
                if(!infer_img.data){
                    std::cout << "---Error--- Can't transform snip img !!! "<< std::endl;
                    break;
                }
                std::cout <<" Snip img size,  H : "<<infer_img.rows<<",   W : "<< infer_img.cols<<",   C: "<< infer_img.channels()<<std::endl;
                std::vector<PaddleOCR::OCRPredictResult> ocr_result = ocr->ocr(infer_img);
                print_result(ocr_result);
                VisualizeBboxes(infer_img, ocr_result, "./temp/ocr_snip_img_result.jpg");
                save_result_to_file(ocr_result,ResTmpDir);
            }
        }

    }

    launchNotepad(ResTmpDir);
    

    delete ocr;
    return EXIT_SUCCESS;
}







