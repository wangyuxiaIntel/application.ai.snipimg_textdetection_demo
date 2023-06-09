#snipimg_textdetection_demo
Get an image by snip gui or clipboard(after win+shift+s), then detection and recognition the texts in the image. Print text and save visible result in /temp subfolder.

#Environment required
OpenVINO, OpenCV, Cmake.
## Set up OpenVINO
Download openvino release and extractor to %OpenVINO_DIR%. link: https://af01p-ir.devtools.intel.com/artifactory/vpu_ci-ir-local/Unified/nightly/integration/vpux-plugin/releases/2023/0/2023.05.30_2101/ci_tag_vpux_rc_20230530_2101/
## Set up OpenCV
Download opencv and install it.link: https://github.com/opencv/opencv/releases
Double click to innstall to %OpenCV_DIR%.

#Build demo
    %OPENVINO_PATH%\setupvars.bat
    %OpenCV_DIR%\build\setup_vars_opencv4.cmd
    build_demos_msvc.bat

The build and release files are in build folder.

#Command params
    -is  Optional. Image source. Default is gui.
    -m_det Optional. Detection model path. xml file.
    -m_rec  Optional. Recognition model path. xml file.

#Run demo
    build\intel64\Release\paddle_ocr_demo.exe
## gui mode
    build\intel64\Release\paddle_ocr_demo.exe -is gui
use built-in gui to snip img.
## clip mode
    build\intel64\Release\paddle_ocr_demo.exe -is clip  
Get image from clipboard. Before using this function, please ensure that there is image data in the clipboard. 
Trigger ocr key: Alt+T. After the two key is pressed down sequentially, execute program will start ocr on clipboard image.
Usage method like: Win+Shift+s -> Alt  -> T .



#TODO
1. WYX TODO in code.
2. Launch text in clipboard mode.
3. Link text that in the same line.
4. Rewrite Keyboard Hook to subdir.