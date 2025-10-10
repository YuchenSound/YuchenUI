/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module (Windows).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#include "Win32WindowImpl.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/core/Assert.h"
#include <windowsx.h>
#include <ShellScalingApi.h>

#pragma comment(lib, "Shcore.lib")

namespace YuchenUI {

//==========================================================================================
// Static Member Initialization

const wchar_t* Win32WindowImpl::WINDOW_CLASS_NAME = L"YuchenUIWindow";
bool Win32WindowImpl::s_classRegistered = false;

//==========================================================================================
// Construction and Destruction

Win32WindowImpl::Win32WindowImpl()
    : m_hwnd(nullptr)
    , m_baseWindow(nullptr)
    , m_windowType(WindowType::Main)
    , m_width(0)
    , m_height(0)
    , m_dpiScale(1.0f)
    , m_isModal(false)
    , m_isVisible(false)
    , m_imeEnabled(true)
{
}

Win32WindowImpl::~Win32WindowImpl()
{
    destroy();
}

//==========================================================================================
// Window Creation and Destruction

bool Win32WindowImpl::create(const WindowConfig& config)
{
    YUCHEN_ASSERT(config.width > 0 && config.height > 0);
    YUCHEN_ASSERT(config.title);
    
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    if (!s_classRegistered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = WINDOW_CLASS_NAME;
        
        if (!RegisterClassExW(&wc))
        {
            return false;
        }
        s_classRegistered = true;
    }
    
    m_windowType = config.type;
    m_width = config.width;
    m_height = config.height;
    
    DWORD style = getWindowStyle(config);
    DWORD exStyle = getWindowExStyle(config);
    
    RECT rect = { 0, 0, config.width, config.height };
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    std::wstring wTitle;
    int len = MultiByteToWideChar(CP_UTF8, 0, config.title, -1, NULL, 0);
    wTitle.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, config.title, -1, &wTitle[0], len);
    
    HWND parentHwnd = nullptr;
    if (config.parent)
    {
        parentHwnd = static_cast<HWND>(config.parent->getNativeWindowHandle());
    }
    
    m_hwnd = CreateWindowExW(
        exStyle,
        WINDOW_CLASS_NAME,
        wTitle.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        parentHwnd,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (!m_hwnd)
    {
        return false;
    }
    
    calculateDpiScale();
    
    return true;
}

void Win32WindowImpl::destroy()
{
    if (m_hwnd)
    {
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

//==========================================================================================
// Window Display

void Win32WindowImpl::show()
{
    if (m_hwnd)
    {
        if (m_windowType == WindowType::Main && !m_isModal)
        {
            centerWindow();
        }
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        m_isVisible = true;
    }
}

void Win32WindowImpl::hide()
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, SW_HIDE);
        m_isVisible = false;
    }
}

void Win32WindowImpl::showModal()
{
    YUCHEN_ASSERT(m_windowType == WindowType::Dialog);
    
    if (!m_hwnd) return;
    
    m_isModal = true;
    
    HWND parentHwnd = GetParent(m_hwnd);
    if (parentHwnd)
    {
        EnableWindow(parentHwnd, FALSE);
    }
    
    centerWindow();
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    m_isVisible = true;
    
    MSG msg;
    while (m_isModal && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (parentHwnd)
    {
        EnableWindow(parentHwnd, TRUE);
        SetForegroundWindow(parentHwnd);
    }
}

void Win32WindowImpl::closeModal()
{
    if (m_isModal)
    {
        m_isModal = false;
        hide();
    }
}

//==========================================================================================
// Window Properties

Vec2 Win32WindowImpl::getSize() const
{
    if (!m_hwnd) return Vec2(m_width, m_height);
    
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    return Vec2(static_cast<float>(rect.right - rect.left),
                static_cast<float>(rect.bottom - rect.top));
}

Vec2 Win32WindowImpl::getPosition() const
{
    if (!m_hwnd) return Vec2();
    
    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    return Vec2(static_cast<float>(rect.left), static_cast<float>(rect.top));
}

bool Win32WindowImpl::isVisible() const
{
    return m_isVisible && m_hwnd && IsWindowVisible(m_hwnd);
}

void* Win32WindowImpl::getNativeHandle() const
{
    return m_hwnd;
}

void Win32WindowImpl::setBaseWindow(BaseWindow* baseWindow)
{
    m_baseWindow = baseWindow;
}

void* Win32WindowImpl::getRenderSurface() const
{
    return m_hwnd;
}

float Win32WindowImpl::getDpiScale() const
{
    return m_dpiScale;
}

Vec2 Win32WindowImpl::mapToScreen(const Vec2& windowPos) const
{
    if (!m_hwnd) return windowPos;
    
    POINT pt = { static_cast<LONG>(windowPos.x), static_cast<LONG>(windowPos.y) };
    ClientToScreen(m_hwnd, &pt);
    return Vec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
}

Rect Win32WindowImpl::getInputMethodCursorWindowRect() const
{
    if (!m_baseWindow) return Rect();
    return m_baseWindow->getInputMethodCursorRect();
}

void Win32WindowImpl::setIMEEnabled(bool enabled)
{
    m_imeEnabled = enabled;

    if (m_hwnd)
    {
        if (enabled)
        {
            ImmAssociateContextEx(m_hwnd, NULL, IACE_DEFAULT);
        }
        else
        {
            ImmAssociateContextEx(m_hwnd, NULL, IACE_IGNORENOCONTEXT);
        }
    }
}

//==========================================================================================
// Window Events

void Win32WindowImpl::onRender()
{
    if (m_baseWindow)
    {
        m_baseWindow->renderContent();
    }
}

void Win32WindowImpl::onResize(int width, int height)
{
    m_width = width;
    m_height = height;
    
    if (m_baseWindow)
    {
        m_baseWindow->onResize(width, height);
    }
}

//==========================================================================================
// Window Style Helpers

DWORD Win32WindowImpl::getWindowStyle(const WindowConfig& config) const
{
    DWORD style = WS_OVERLAPPEDWINDOW;
    
    if (config.type == WindowType::Dialog)
    {
        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    }
    else if (config.type == WindowType::ToolWindow)
    {
        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    }
    
    if (!config.resizable)
    {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    
    return style;
}

DWORD Win32WindowImpl::getWindowExStyle(const WindowConfig& config) const
{
    DWORD exStyle = WS_EX_APPWINDOW;
    
    if (config.type == WindowType::Dialog)
    {
        exStyle |= WS_EX_DLGMODALFRAME;
    }
    else if (config.type == WindowType::ToolWindow)
    {
        exStyle |= WS_EX_TOOLWINDOW;
        if (config.floating)
        {
            exStyle |= WS_EX_TOPMOST;
        }
    }
    
    return exStyle;
}

void Win32WindowImpl::centerWindow()
{
    if (!m_hwnd) return;
    
    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    
    SetWindowPos(m_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Win32WindowImpl::calculateDpiScale()
{
    if (!m_hwnd)
    {
        m_dpiScale = 1.0f;
        return;
    }

    UINT dpi = GetDpiForWindow(m_hwnd);
    m_dpiScale = static_cast<float>(dpi) / 96.0f;
}

//==========================================================================================
// Window Procedure

LRESULT CALLBACK Win32WindowImpl::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win32WindowImpl* impl = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        impl = reinterpret_cast<Win32WindowImpl*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
    }
    else
    {
        impl = reinterpret_cast<Win32WindowImpl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (!impl || !impl->m_baseWindow)
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_CLOSE:
        if (impl->m_windowType == WindowType::ToolWindow)
        {
            impl->m_baseWindow->hide();
        }
        else
        {
            impl->m_baseWindow->closeWithResult(WindowContentResult::Close);
        }
        return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        impl->onResize(width, height);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        impl->onRender();
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DPICHANGED:
    {
        impl->calculateDpiScale();
        
        RECT* rect = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hwnd, NULL,
            rect->left, rect->top,
            rect->right - rect->left,
            rect->bottom - rect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_IME_SETCONTEXT:
        if (wParam)
        {
            lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_IME_STARTCOMPOSITION:
    {
        MSG message = { hwnd, msg, wParam, lParam };
        impl->m_baseWindow->handleNativeEvent(&message);

        HIMC hImc = ImmGetContext(hwnd);
        if (hImc && impl->m_imeEnabled)
        {
            Rect cursorRect = impl->getInputMethodCursorWindowRect();

            COMPOSITIONFORM cf;
            cf.dwStyle = CFS_POINT;
            cf.ptCurrentPos.x = static_cast<LONG>(cursorRect.x);
            cf.ptCurrentPos.y = static_cast<LONG>(cursorRect.y);
            ImmSetCompositionWindow(hImc, &cf);

            CANDIDATEFORM caf;
            caf.dwStyle = CFS_CANDIDATEPOS;
            caf.ptCurrentPos.x = static_cast<LONG>(cursorRect.x);
            caf.ptCurrentPos.y = static_cast<LONG>(cursorRect.y + cursorRect.height);
            ImmSetCandidateWindow(hImc, &caf);

            LOGFONTA lf = {};
            lf.lfHeight = -static_cast<LONG>(14.0f * impl->getDpiScale());
            lf.lfCharSet = DEFAULT_CHARSET;
            strcpy_s(lf.lfFaceName, "Microsoft YaHei");
            ImmSetCompositionFontA(hImc, &lf);

            ImmReleaseContext(hwnd, hImc);
        }
        return 0;
    }

    case WM_IME_COMPOSITION:
    {
        MSG message = { hwnd, msg, wParam, lParam };
        impl->m_baseWindow->handleNativeEvent(&message);

        if (lParam & (GCS_COMPSTR | GCS_RESULTSTR))
        {
            HIMC hImc = ImmGetContext(hwnd);
            if (hImc && impl->m_imeEnabled)
            {
                Rect cursorRect = impl->getInputMethodCursorWindowRect();

                COMPOSITIONFORM cf;
                cf.dwStyle = CFS_POINT;
                cf.ptCurrentPos.x = static_cast<LONG>(cursorRect.x);
                cf.ptCurrentPos.y = static_cast<LONG>(cursorRect.y);
                ImmSetCompositionWindow(hImc, &cf);

                CANDIDATEFORM caf;
                caf.dwStyle = CFS_CANDIDATEPOS;
                caf.ptCurrentPos.x = static_cast<LONG>(cursorRect.x);
                caf.ptCurrentPos.y = static_cast<LONG>(cursorRect.y + cursorRect.height);
                ImmSetCandidateWindow(hImc, &caf);

                ImmReleaseContext(hwnd, hImc);
            }
        }
        return 0;
    }

    case WM_IME_ENDCOMPOSITION:
    {
        MSG message = { hwnd, msg, wParam, lParam };
        impl->m_baseWindow->handleNativeEvent(&message);
        return 0;
    }

    case WM_IME_NOTIFY:
    {
        MSG message = { hwnd, msg, wParam, lParam };
        impl->m_baseWindow->handleNativeEvent(&message);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    case WM_IME_CHAR:
        return 0;

    default:
    {
        MSG message = { hwnd, msg, wParam, lParam };
        impl->m_baseWindow->handleNativeEvent(&message);
        break;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//==========================================================================================
// Factory

WindowImpl* WindowImplFactory::create()
{
    return new Win32WindowImpl();
}

} // namespace YuchenUI
