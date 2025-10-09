#pragma once

#include "YuchenUI/events/EventManager.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"
#include <Windows.h>
#include <imm.h>
#include <vector>

#pragma comment(lib, "Imm32.lib")

namespace YuchenUI {

class Win32EventManager : public EventManager {
public:
    explicit Win32EventManager(HWND hwnd);
    virtual ~Win32EventManager();

    bool initialize() override;
    void destroy() override;
    bool isInitialized() const override;

    bool hasEvents() const override;
    Event getNextEvent() override;
    void clearEvents() override;
    size_t getEventCount() const override;

    void setEventCallback(EventCallback callback) override;
    void clearEventCallback() override;
    bool hasEventCallback() const override;

    void handleNativeEvent(void* event) override;

    bool isKeyPressed(KeyCode key) const override;
    bool isMouseButtonPressed(MouseButton button) const override;
    Vec2 getMousePosition() const override;
    KeyModifiers getCurrentModifiers() const override;

    void enableTextInput() override;
    void disableTextInput() override;
    bool isTextInputEnabled() const override;

private:
    void handleWindowsMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleKeyEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleMouseButtonEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleMouseMoveEvent(WPARAM wParam, LPARAM lParam);
    void handleMouseWheelEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    void handleCharEvent(WPARAM wParam);
    void handleImeStartComposition();
    void handleImeComposition(LPARAM lParam);
    void handleImeEndComposition();
    void handleImeNotify(WPARAM wParam);

    KeyCode mapVirtualKey(WPARAM vk) const;
    MouseButton mapMouseButton(UINT msg) const;
    KeyModifiers extractModifiers() const;
    Vec2 getMousePositionFromLParam(LPARAM lParam) const;
    double getCurrentTime() const;
    void pushEvent(const Event& event);

    HWND m_hwnd;
    EventQueue<Config::Events::EVENT_QUEUE_SIZE> m_eventQueue;
    EventCallback m_eventCallback;
    KeyStateTracker m_keyTracker;
    MouseStateTracker m_mouseTracker;
    bool m_isInitialized;
    bool m_textInputEnabled;
    bool m_imeComposing;
    std::vector<char> m_imeCompositionBuffer;
};

}
