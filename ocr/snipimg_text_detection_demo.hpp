#pragma once

#include <string>
#include <vector>
#include <direct.h>

#include "gflags/gflags.h"
#include <windows.h>


static const char help_message[] = "Print a usage message.";
static const char ppocr_detection_model_message[]="Required. Path to PPOCR text detection model(.xml) file.";
static const char ppocr_recognition_model_message[]="Required. Path to PPOCR text recognition model(.xml) file.";
static const char ppocr_recognition_dict_message[]="Required. Path to a dictionary(.txt) that content charactor to parse the recognition result from index to string.";

//Model Config
DEFINE_bool(h, false, help_message);
DEFINE_string(m_det, "./ocr/models/en_PP-OCRv3_det_infer/xml/inference.xml", ppocr_detection_model_message);
DEFINE_string(m_rec, "./ocr/models/en_PP-OCRv3_rec_infer/xml/inference.xml", ppocr_recognition_model_message);
DEFINE_string(d_det, "CPU", "detection model run on CPU");
DEFINE_string(d_rec, "CPU", "detection model run on CPU");

DEFINE_bool(dymc_det, true, "WYX TODO");
DEFINE_bool(dymc_rec, true, "WYX TODO");
DEFINE_bool(upr_det, false, "WYX TODO");
DEFINE_bool(upr_rec, false, "WYX TODO");

//WYX TODO change to relative path
//WYX TODO verify the last dimension of the model output is same.
DEFINE_string(rec_dict, "./ocr/en_dict.txt", ppocr_recognition_dict_message);

//Snip config
DEFINE_string(is, "gui", "Required. Target image source. If gui, use Use the built-in GUI interface to take screenshots. If clip, Read the image of the clipboard after reading wss+atd");



static void showUsage() {
    // WYX TODO
}

