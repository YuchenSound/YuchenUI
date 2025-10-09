#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/windows/IWindowContent.h"
#include "YuchenUI/windows/BaseWindow.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class Window;

class WindowManager {
public:
    static WindowManager& getInstance();
    
    bool initialize();
    void destroy();
    bool isInitialized() const { return m_isInitialized; }
    
    void run();
    void quit();

    template<typename ContentType, typename... Args>
    BaseWindow* createMainWindow(int width, int height, const char* title, Args&&... args) {
        if (!m_isInitialized) {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto mainWindow = std::make_unique<BaseWindow>(WindowType::Main);
        if (!mainWindow->createWithContent<ContentType>(width, height, title, nullptr, std::forward<Args>(args)...)) {
            return nullptr;
        }
        
        BaseWindow* mainWindowPtr = mainWindow.get();
        m_mainWindows.push_back(std::move(mainWindow));
        registerWindow(mainWindowPtr);
        
        return mainWindowPtr;
    }
    
    size_t getMainWindowCount() const { return m_mainWindows.size(); }
    void closeMainWindow(BaseWindow* mainWindow);
    bool isMainWindow(const BaseWindow* window) const;
    
    template<typename ContentType, typename... Args>
    BaseWindow* createDialog(int width, int height, const char* title, Window* parent, Args&&... args) {
        if (!m_isInitialized) {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto dialog = std::make_unique<BaseWindow>(WindowType::Dialog);
        if (!dialog->createWithContent<ContentType>(width, height, title, parent, std::forward<Args>(args)...)) {
            return nullptr;
        }
        
        BaseWindow* dialogPtr = dialog.get();
        m_dialogs.push_back(std::move(dialog));
        registerWindow(dialogPtr);
        
        return dialogPtr;
    }
    
    template<typename ContentType, typename... Args>
    BaseWindow* createToolWindow(int width, int height, const char* title, Window* parent, Args&&... args) {
        if (!m_isInitialized) {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto toolWindow = std::make_unique<BaseWindow>(WindowType::ToolWindow);
        if (!toolWindow->createWithContent<ContentType>(width, height, title, parent, std::forward<Args>(args)...)) {
            return nullptr;
        }
        
        BaseWindow* toolWindowPtr = toolWindow.get();
        m_toolWindows.push_back(std::move(toolWindow));
        registerWindow(toolWindowPtr);
        
        return toolWindowPtr;
    }
    
    void closeDialog(BaseWindow* dialog);
    void closeToolWindow(BaseWindow* toolWindow);
    void closeAllWindows();
    
    void* getSharedRenderDevice() const { return m_sharedRenderDevice; }
    
    void registerWindow(Window* window);
    void unregisterWindow(Window* window);
    void scheduleDialogDestruction(BaseWindow* dialog);

private:
    WindowManager();
    ~WindowManager();
    void processScheduledDestructions();
    std::vector<BaseWindow*> m_scheduledDialogDestructions;
    bool createSharedRenderDevice();
    void cleanupResources();
    
    static WindowManager* s_instance;
    
    bool m_isInitialized;
    bool m_isRunning;
    
    std::vector<std::unique_ptr<BaseWindow>> m_mainWindows;
    
    void* m_sharedRenderDevice;
    
    std::vector<Window*> m_allWindows;
    std::vector<std::unique_ptr<BaseWindow>> m_dialogs;
    std::vector<std::unique_ptr<BaseWindow>> m_toolWindows;
    
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
};

}
