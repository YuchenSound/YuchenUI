// Win32WindowImpl.h
#pragma once

#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/core/Types.h"
#include <Windows.h>
#include <string>

namespace YuchenUI {

class BaseWindow;

class Win32WindowImpl : public WindowImpl {
public:
    Win32WindowImpl();
    virtual ~Win32WindowImpl();
    
    bool create(const WindowConfig& config) override;
    void destroy() override;
    void show() override;
    void hide() override;
    void showModal() override;
    void closeModal() override;
    Vec2 getSize() const override;
    Vec2 getPosition() const override;
    bool isVisible() const override;
    void* getNativeHandle() const override;
    
    void setBaseWindow(BaseWindow* baseWindow) override;
    void* getRenderSurface() const override;
    float getDpiScale() const override;
    Vec2 mapToScreen(const Vec2& windowPos) const override;
    Rect getInputMethodCursorWindowRect() const override;
    void setIMEEnabled(bool enabled) override;
    
    void onRender();
    void onResize(int width, int height);
    
    BaseWindow* getBaseWindow() const { return m_baseWindow; }

private:
    DWORD getWindowStyle(const WindowConfig& config) const;
    DWORD getWindowExStyle(const WindowConfig& config) const;
    void centerWindow();
    void calculateDpiScale();
    
    HWND m_hwnd;
    BaseWindow* m_baseWindow;
    WindowType m_windowType;
    int m_width;
    int m_height;
    float m_dpiScale;
    bool m_isModal;
    bool m_isVisible;
    bool m_imeEnabled;
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static const wchar_t* WINDOW_CLASS_NAME;
    static bool s_classRegistered;
};

}
