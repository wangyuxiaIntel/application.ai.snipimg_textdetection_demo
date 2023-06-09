#pragma once
#include <windows.h>
#include <windowsx.h>
#include <opencv2/opencv.hpp>

namespace SnipGUI{

RECT NormalizeRect(RECT inputRect);

class MainWindow
{
public:
    static HRESULT Create(
        _In_ PCSTR pWindowName,
        _In_ SIZE size,
        _In_ POINT pt,
        _Outptr_ MainWindow** ppButton);//创建button窗口
protected:
    static bool EnsureWndClass();//定义button窗口类

    static LRESULT CALLBACK WindowProc(
        _In_ HWND hwnd,
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);//定义button窗口的回调函数

    static constexpr PCSTR c_szLayeredClassName="LayeredWindow";
    static bool s_classRegistered;

    HRESULT Initialize(_In_ PCSTR pWindowName);//定义和创建窗口，将窗口handle赋值给m_hwnd

public:
    MainWindow(
        _In_ SIZE size,
        _In_ POINT pt)
        :
        m_size(size),
        m_pt(pt)
    {
    }

    virtual ~MainWindow()
    {
        if (m_hwnd != NULL)
        {
            DestroyWindow(m_hwnd);
        }
    }

protected:
    void OnClicked();

protected:
    HWND m_hwnd = NULL;
    SIZE m_size;
    POINT m_pt;
};


class SnipWindow
{
public:
    static HRESULT Create(
        _In_ PCSTR pWindowName,
        _Outptr_ SnipWindow** ppWindow
    );//create snip window
// protected:    
    bool EnsureWndClass();
    
    static LRESULT CALLBACK WindowProc(
        _In_ HWND hwnd,
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
    );

    static constexpr PCSTR c_szLayeredClassName="LayeredSnip";
    static bool s_classRegistered;

    HRESULT Initialize(_In_ PCSTR pWindowName);
    HRESULT CaptureZone(RECT captureRect);
    HRESULT SnipWindow::MouseDownProc(HWND hwnd, LPARAM lParam);
    HRESULT SnipWindow::MouseUpProc(HWND hwnd, LPARAM lParam);
    HRESULT SnipWindow::MouseMoveProc(HWND hwnd, LPARAM lParam);
    HRESULT SnipWindow::WM_PAINTProc(HWND hwnd, LPARAM lParam);

public:
    SnipWindow(){

    }
    virtual ~SnipWindow(){
        if(m_hwnd!=NULL){
            DestroyWindow(m_hwnd);
        }
    }

// protected:
public:
    HWND m_hwnd=NULL;
    bool m_SnipMouseDown=false;
    RECT m_snipRect={};

};

}