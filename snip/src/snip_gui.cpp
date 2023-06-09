#include "snip_gui.hpp"

// extern HINSTANCE g_hInstance;
// extern HWND g_SnipHwnd;
// extern cv::Mat g_snip_img;
// extern bool g_wait_for_snip;

namespace SnipGUI{

extern HINSTANCE g_hInstance;
extern HWND g_SnipHwnd;
extern cv::Mat g_snip_img;
extern bool g_wait_for_snip;

bool MainWindow::s_classRegistered = false;
bool SnipWindow::s_classRegistered = false;

RECT NormalizeRect(RECT inputRect)
{
    RECT outputRect;
    outputRect.left = inputRect.left > inputRect.right ? inputRect.right : inputRect.left;
    outputRect.top = inputRect.top > inputRect.bottom ? inputRect.bottom : inputRect.top;
    outputRect.right= inputRect.left < inputRect.right ? inputRect.right : inputRect.left;
    outputRect.bottom = inputRect.top < inputRect.bottom ? inputRect.bottom : inputRect.top;
    return outputRect;
}


/*static*/
HRESULT MainWindow::Create(
    _In_ PCSTR pWindowName,
    _In_ SIZE size,
    _In_ POINT pt,
    _Outptr_ MainWindow** ppButton)
{
    HRESULT hr = S_OK;
    MainWindow* pButton = new MainWindow(size, pt);
    if (pButton != nullptr)
    {
        hr = pButton->Initialize(pWindowName);
        if (SUCCEEDED(hr))
        {
            ShowWindow(pButton->m_hwnd, SW_SHOW);

            *ppButton = pButton;
            pButton = nullptr;
        }

        delete pButton;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*static*/
bool MainWindow::EnsureWndClass()
{
    if (!s_classRegistered)
    {
        WNDCLASS wndClass = {};

        wndClass.lpfnWndProc = MainWindow::WindowProc;
        wndClass.hInstance = g_hInstance;
        wndClass.lpszClassName = c_szLayeredClassName;

        if (RegisterClass(&wndClass))
        {
            s_classRegistered = true;
        }
    }

    return s_classRegistered;
}

HRESULT MainWindow::Initialize(_In_ PCSTR pWindowName)
{
    if (!EnsureWndClass())
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_hwnd = CreateWindowEx(0,              // Optional window styles.
        c_szLayeredClassName,
        pWindowName,  
        WS_OVERLAPPEDWINDOW, // Window style
        m_pt.x,
        m_pt.y,
        m_size.cx,
        m_size.cy,
        NULL,     // Parent window    
        NULL,           // Menu
        g_hInstance,    // Instance handle
        this);//must set this, otherwise pThis can not get in WindowProc

    return true;
}


/*static*/
LRESULT CALLBACK MainWindow::WindowProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    MainWindow* pThis = nullptr;

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT* pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(pCS->lpCreateParams);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

        CreateWindow( TEXT("button"),
				TEXT("snip a zone"),
				WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
				pThis->m_pt.x+38,
				pThis->m_pt.y+5,
				150, 
                30,
				hwnd,
				(HMENU)0,
				((LPCREATESTRUCT) lParam)->hInstance,
				NULL);
    }
    else
    {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis != nullptr)
    {
        // Assert(pThis->m_hwnd == hwnd);
        switch (uMsg)
        {
            case WM_DESTROY:
            {
                hwnd = NULL;
                PostQuitMessage(0);
            }
            return 0;

            //case WM_LBUTTONUP:
            case WM_COMMAND:
            {
                pThis->OnClicked();
            }
            return 0;
        }
        
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



/* Override */
void MainWindow::OnClicked()
{    
    HWND hwndDesktop = GetDesktopWindow();
    RECT crDesktop;
    GetClientRect(hwndDesktop, &crDesktop);
    SetWindowPos(
        g_SnipHwnd,
        NULL,
        crDesktop.left,
        crDesktop.top,
        (crDesktop.right - crDesktop.left),
        (crDesktop.bottom - crDesktop.top),
        NULL);
    ShowWindow(g_SnipHwnd, SW_SHOW);
}

/*static*/
bool SnipWindow::EnsureWndClass(){
    if(!s_classRegistered){
        //注册区域窗口
        //WNDCLASSW 窗口结构体类
        WNDCLASS SnipWndClass = {};
        SnipWndClass.lpfnWndProc = SnipWindow::WindowProc;
        SnipWndClass.hInstance = g_hInstance;
        SnipWndClass.lpszClassName = c_szLayeredClassName;

        if(RegisterClass(&SnipWndClass)){
            s_classRegistered=true;
        }
    }
    return s_classRegistered;
}

HRESULT SnipWindow::Initialize(_In_ PCSTR pWindowName){
    if(!EnsureWndClass()){
        return HRESULT_FROM_WIN32(GetLastError());
    }

    m_hwnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            c_szLayeredClassName,
            pWindowName,
            WS_POPUPWINDOW,
            0,
            0,
            100,
            100,
            NULL,
            NULL,
            g_hInstance,
            this);

    COLORREF colorref = 0x000000FF;
    SetLayeredWindowAttributes(
        m_hwnd,
        colorref,
        125,
        LWA_ALPHA | LWA_COLORKEY);

}


/*static*/
HRESULT SnipWindow::Create(
    _In_ PCSTR pWindowName,
    _Outptr_ SnipWindow** ppWindow)
{
    HRESULT hr = S_OK;
    SnipWindow* pWindowObj=new SnipWindow();
    if(pWindowObj!=nullptr){
        hr = pWindowObj->Initialize(pWindowName);
        if (SUCCEEDED(hr))
        {
            *ppWindow = pWindowObj;
            pWindowObj = nullptr;
        }

        delete pWindowObj;
    }
    else{
        hr=E_OUTOFMEMORY;
    }
    return hr;
}

/*static*/
LRESULT CALLBACK SnipWindow::WindowProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{   
    SnipWindow* pThis = nullptr;

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT* pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<SnipWindow*>(pCS->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<SnipWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    HRESULT hr = S_OK;
    if (pThis != nullptr){
        switch (uMsg)
        {
            case WM_PAINT:
            {
                hr=pThis->WM_PAINTProc(hwnd,lParam);
            }
            return 0;

            case WM_LBUTTONDOWN://点击鼠标的消息
            {
                hr=pThis->MouseDownProc(hwnd,lParam);
            }
            return 0;

            case WM_LBUTTONUP:
            {
                hr=pThis->MouseUpProc(hwnd,lParam);
            }
            return 0;

            case WM_MOUSEMOVE:
            {
                hr=pThis->MouseMoveProc(hwnd,lParam);
            }
            return 0;

            case WM_ERASEBKGND:
            {

            }
            return 0;
        }  
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    
}    

HRESULT SnipWindow::WM_PAINTProc(HWND hwnd, LPARAM lParam){
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT crDesktop;
    GetClientRect(GetDesktopWindow(), &crDesktop);

    COLORREF colorref = 0x00818181;
    HBRUSH brush = CreateSolidBrush(colorref);
    FillRect(hdc, &ps.rcPaint, brush);

    colorref = 0x000000FF;
    brush = CreateSolidBrush(colorref);
    FillRect(hdc, &m_snipRect, brush);

    EndPaint(hwnd, &ps);
    return S_OK;
}

HRESULT SnipWindow::MouseDownProc(HWND hwnd, LPARAM lParam){
    SetCapture(hwnd);
    m_SnipMouseDown = true;
    POINT clickPos;
    clickPos.x = GET_X_LPARAM(lParam);
    clickPos.y = GET_Y_LPARAM(lParam);

    m_snipRect.left = clickPos.x;
    m_snipRect.top = clickPos.y;
    return S_OK;
}
HRESULT SnipWindow::MouseUpProc(HWND hwnd,LPARAM lParam){
    m_SnipMouseDown = false;
    ReleaseCapture();
    ShowWindow(hwnd, SW_HIDE);
    CaptureZone(NormalizeRect(m_snipRect));
    m_snipRect={};
    return S_OK;
}
HRESULT SnipWindow::MouseMoveProc(HWND hwnd,LPARAM lParam){
    if (m_SnipMouseDown)
    {
        POINT mousePos;
        mousePos.x = GET_X_LPARAM(lParam);
        mousePos.y = GET_Y_LPARAM(lParam);

        RECT oldSnipRect = m_snipRect;

        m_snipRect.right = mousePos.x;
        m_snipRect.bottom = mousePos.y;

        InvalidateRect(hwnd, &m_snipRect, 0);
        InvalidateRect(hwnd, &oldSnipRect, 0);
    }
    return S_OK;
}

HRESULT SnipWindow::CaptureZone(RECT captureRect)
{
    HBITMAP hbitmap;

    HDC hdcDesktop = GetWindowDC(GetDesktopWindow());
    HDC hdcScreen = GetDC(NULL);
    HDC hdcDest = CreateCompatibleDC(hdcDesktop);

    hbitmap = CreateCompatibleBitmap(
        hdcDesktop,
        (captureRect.right - captureRect.left),
        (captureRect.bottom - captureRect.top));
    HGDIOBJ hbmpSave = SelectObject(hdcDest, hbitmap);

    BitBlt(
        hdcDest,
        0,
        0,
        (captureRect.right - captureRect.left),
        (captureRect.bottom - captureRect.top),
        hdcScreen,
        captureRect.left,
        captureRect.top,
        SRCCOPY);
    LPVOID m_screenshotData = nullptr;
    m_screenshotData = new char[(captureRect.right - captureRect.left) * (captureRect.bottom - captureRect.top) * 4];
    GetBitmapBits(hbitmap, (captureRect.right - captureRect.left) * (captureRect.bottom - captureRect.top) * 4, m_screenshotData);

    cv::Mat screenshot((captureRect.bottom - captureRect.top), (captureRect.right - captureRect.left), CV_8UC4, m_screenshotData);
    //cv::Mat screenshot(2, 2, CV_8UC3, cv::Scalar(0, 0, 255));
    // cv::imwrite("D:/OpenVINO_Model_Zoo/temp_vs_pro/snip_demo/shot_temp.jpg", screenshot);

    // P_snip_img=&screenshot;
    g_snip_img=screenshot.clone();
    g_wait_for_snip=false;
    
    delete [] m_screenshotData;

    SelectObject(hdcDest, hbmpSave);
    DeleteDC(hdcDest);

    DeleteDC(hdcScreen);

    DeleteDC(hdcDesktop);

    //g_count = 0;

    return S_OK;
}


}//namespace SnipGUI