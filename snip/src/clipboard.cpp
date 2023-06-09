#include "clipboard.hpp"

//This code come from https://github.com/hiroi-sora/PaddleOCR-json/blob/main/cpp/src/task_win32.cpp
// 从剪贴板读入一张图片，返回Mat。注意flag对剪贴板内存图片无效，仅对剪贴板路径图片有效。
cv::Mat imread_clipboard()
{
    // 参考文档： https://docs.microsoft.com/zh-cn/windows/win32/dataxchg/using-the-clipboard

    // 尝试打开剪贴板，锁定，防止其他应用程序修改剪贴板内容
    if (!OpenClipboard(NULL))
    {
        std::cout<<"---Clipboard---     Failed to open clipboard."<<std::endl; // 报告状态：剪贴板打开失败
    }
    else
    {
        static unsigned int auPriorityList[] = {
            // 允许读入的剪贴板格式：
            CF_BITMAP, // 位图
            CF_HDROP,  // 文件列表句柄（文件管理器选中文件复制）
        };
        int auPriorityLen = sizeof(auPriorityList) / sizeof(auPriorityList[0]);  // 列表长度
        int uFormat = GetPriorityClipboardFormat(auPriorityList, auPriorityLen); // 获取当前剪贴板内容的格式
        // 根据格式分配不同任务。
        //     若任务成功，释放全部资源，关闭剪贴板，返回图片mat。
        //     若任务失败，释放已打开的资源和锁，报告状态，跳出switch，统一关闭剪贴板和返回空mat
        switch (uFormat)
        {
        case CF_BITMAP:
        {                                                     // 1. 位图 ===================================================================
            HBITMAP hbm = (HBITMAP)GetClipboardData(uFormat); // 1.1. 从剪贴板中录入指针，得到文件句柄
            if (hbm)
            {
                // GlobalLock(hbm); // 返回值总是无效的，读位图似乎不需要锁？
                // https://social.msdn.microsoft.com/Forums/vstudio/en-US/d2a6aa71-68d7-4db0-8b1f-5d1920f9c4ce/globallock-and-dib-transform-into-hbitmap-issue?forum=vcgeneral
                BITMAP bmp;                           // 存放指向缓冲区的指针，缓冲区接收有关指定图形对象的信息
                GetObject(hbm, sizeof(BITMAP), &bmp); // 1.2. 获取图形对象的信息（不含图片内容本身）
                if (!hbm)
                {
                    std::cout<<"---Clipboard---     Failed to retrieve clipboard image."<<std::endl; // 报告状态：检索图形对象信息失败
                    break;
                }
                int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8; // 根据色深计算通道数，32bit为4，24bit为3
                // 1.3. 将句柄hbm中的位图复制到缓冲区
                long sz = bmp.bmHeight * bmp.bmWidth * nChannels;                                // 图片大小（字节）
                cv::Mat mat(cv::Size(bmp.bmWidth, bmp.bmHeight), CV_MAKETYPE(CV_8U, nChannels)); // 创造空矩阵，传入位图大小和深度
                long getsz = GetBitmapBits(hbm, sz, mat.data);                                   // 将句柄hbm中sz个字节复制到缓冲区img.data
                if (!getsz)
                {
                    std::cout<<"---Clipboard---     Failed to obtain bitmap data. "<<std::endl; // 报告状态：获取位图数据失败
                    break;
                }
                CloseClipboard(); // 释放资源
                // 1.4. 返回合适的通道
                if (mat.data)
                {
                    if (nChannels == 1 || nChannels == 3)
                    { // 1或3通道，PPOCR可识别，直接返回
                        return mat;
                    }
                    else if (nChannels == 4)
                    { // 4通道，PPOCR不可识别，删去alpha转3通道再返回
                        cv::Mat mat_c3;
                        cv::cvtColor(mat, mat_c3, cv::COLOR_BGRA2BGR); // 色彩空间转换
                        return mat_c3;
                    }
                    std::cout<<"---Clipboard---     Unsupported channel: "<< nChannels <<std::endl; // 报告状态：通道数异常
                    break;
                }
                // 理论上上面 !getsz 已经 break 了，不会走到这里。保险起见再报告一次
                std::cout<<"---Clipboard---     Failed to obtain bitmap data. "<<std::endl; // 报告状态：获取位图数据失败
                break;
            }
            std::cout<<"---Clipboard---     Failed to obtain clipboard data"<<std::endl; // 报告状态：获取剪贴板数据失败
            break;
        }
        case NULL:                                              // 剪贴板为空
            std::cout<<"---Clipboard---     Clipboard is empty "<<std::endl; // 报告状态：剪贴板为空
            break;                                                  // 其它不支持的格式
        default:                                                  // 未知
            std::cout<<"---Clipboard---     The format of the clipboard content is not supported."<<std::endl; // 报告状态： 剪贴板的格式不支持
            break;
        }
        CloseClipboard(); // 为break的情况关闭剪贴板，使其他窗口能够继续访问剪贴板。
    }
    return cv::Mat();
}