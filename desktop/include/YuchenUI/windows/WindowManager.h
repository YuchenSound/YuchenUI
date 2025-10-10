/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Windows module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"
#include "YuchenUI/windows/BaseWindow.h"
#include <vector>
#include <memory>

namespace YuchenUI {

class Window;

//==========================================================================================
/**
    Singleton manager for all windows in the application.
    
    WindowManager handles window lifecycle, event loop, and shared rendering resources.
    It manages three types of windows: main windows, dialogs, and tool windows.
    
    The application runs until all main windows are closed.
    
    Example usage:
    @code
    WindowManager& wm = WindowManager::getInstance();
    wm.initialize();
    
    auto* mainWindow = wm.createMainWindow<MyContent>(800, 600, "My App");
    mainWindow->show();
    
    wm.run();
    @endcode
    
    @see BaseWindow, IWindowContent
*/
class WindowManager
{
public:
    //======================================================================================
    /** Returns the singleton instance. */
    static WindowManager& getInstance();
    
    //======================================================================================
    /** Initializes the window manager and graphics system.
        
        Must be called before creating any windows.
        
        @returns True if initialization succeeded, false otherwise
    */
    bool initialize();
    
    /** Destroys all windows and releases resources. */
    void destroy();
    
    /** Returns true if the window manager is initialized. */
    bool isInitialized() const { return m_isInitialized; }
    
    //======================================================================================
    /** Starts the application event loop.
        
        This method blocks until quit() is called or all main windows are closed.
    */
    void run();
    
    /** Requests the application to quit.
        
        The event loop will exit after processing pending events.
    */
    void quit();

    //======================================================================================
    /** Creates a main application window with content.
        
        Main windows keep the application running. When all main windows close,
        the application quits.
        
        @tparam ContentType  The IWindowContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param args          Arguments forwarded to ContentType constructor
        
        @returns Pointer to the created window, or nullptr on failure
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createMainWindow (int width, int height, const char* title, Args&&... args)
    {
        if (!m_isInitialized)
        {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto mainWindow = std::make_unique<BaseWindow> (WindowType::Main);
        if (!mainWindow->createWithContent<ContentType> (width, height, title, nullptr, std::forward<Args>(args)...))
        {
            return nullptr;
        }
        
        BaseWindow* mainWindowPtr = mainWindow.get();
        m_mainWindows.push_back (std::move (mainWindow));
        registerWindow (mainWindowPtr);
        
        return mainWindowPtr;
    }
    
    /** Returns the number of main windows currently open. */
    size_t getMainWindowCount() const { return m_mainWindows.size(); }
    
    /** Closes a main window.
        
        If this is the last main window, the application will quit.
        
        @param mainWindow  The window to close
    */
    void closeMainWindow (BaseWindow* mainWindow);
    
    /** Returns true if the specified window is a main window. */
    bool isMainWindow (const BaseWindow* window) const;
    
    //======================================================================================
    /** Creates a modal dialog window with content.
        
        Dialogs are temporary windows typically used for user interaction.
        
        @tparam ContentType  The IWindowContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param parent        Parent window for the dialog
        @param args          Arguments forwarded to ContentType constructor
        
        @returns Pointer to the created dialog, or nullptr on failure
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createDialog (int width, int height, const char* title, Window* parent, Args&&... args)
    {
        if (!m_isInitialized)
        {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto dialog = std::make_unique<BaseWindow> (WindowType::Dialog);
        if (!dialog->createWithContent<ContentType> (width, height, title, parent, std::forward<Args>(args)...))
        {
            return nullptr;
        }
        
        BaseWindow* dialogPtr = dialog.get();
        m_dialogs.push_back (std::move (dialog));
        registerWindow (dialogPtr);
        
        return dialogPtr;
    }
    
    /** Creates a tool window with content.
        
        Tool windows are typically used for auxiliary UI such as palettes or inspectors.
        
        @tparam ContentType  The IWindowContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param parent        Parent window for the tool window
        @param args          Arguments forwarded to ContentType constructor
        
        @returns Pointer to the created tool window, or nullptr on failure
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createToolWindow (int width, int height, const char* title, Window* parent, Args&&... args)
    {
        if (!m_isInitialized)
        {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto toolWindow = std::make_unique<BaseWindow> (WindowType::ToolWindow);
        if (!toolWindow->createWithContent<ContentType> (width, height, title, parent, std::forward<Args>(args)...))
        {
            return nullptr;
        }
        
        BaseWindow* toolWindowPtr = toolWindow.get();
        m_toolWindows.push_back (std::move (toolWindow));
        registerWindow (toolWindowPtr);
        
        return toolWindowPtr;
    }
    
    /** Closes a dialog window.
        
        @param dialog  The dialog to close
    */
    void closeDialog (BaseWindow* dialog);
    
    /** Closes a tool window.
        
        @param toolWindow  The tool window to close
    */
    void closeToolWindow (BaseWindow* toolWindow);
    
    /** Closes all windows (main, dialog, and tool windows). */
    void closeAllWindows();
    
    //======================================================================================
    /** Returns the shared rendering device handle.
        
        All windows share a single rendering device for efficiency.
    */
    void* getSharedRenderDevice() const { return m_sharedRenderDevice; }
    
    /** Registers a window with the manager. */
    void registerWindow (Window* window);
    
    /** Unregisters a window from the manager. */
    void unregisterWindow (Window* window);
    
    /** Schedules a dialog for destruction after the current event loop iteration.
        
        This is used internally for modal dialogs that need to close themselves.
        
        @param dialog  The dialog to destroy
    */
    void scheduleDialogDestruction (BaseWindow* dialog);
    
    //======================================================================================
    /**
        Returns all registered windows.
        
        Provides access to the window collection for platform event loop implementations.
        The platform code uses this to iterate over windows for rendering and event dispatch.
        
        @returns Const reference to vector of all registered windows
        
        @note This method is public to allow platform layer access but should not be
              called by application code. It is part of the internal platform abstraction.
    */
    const std::vector<Window*>& getAllWindows() const;
    
    /**
        Processes dialogs scheduled for destruction.
        
        Dialogs that complete their modal event loop are scheduled for destruction
        rather than destroyed immediately. This method processes the destruction queue
        and must be called from the event loop after each frame.
        
        @note This method is public to allow platform layer access but should not be
              called by application code. It is part of the internal platform abstraction.
    */
    void processScheduledDestructions();

private:
    //======================================================================================
    WindowManager();
    ~WindowManager();
    
    std::vector<BaseWindow*> m_scheduledDialogDestructions;
    
    /** Creates the shared rendering device. */
    bool createSharedRenderDevice();
    
    /** Initializes platform-specific menu backend. */
    void initializeMenuBackend();
    
    /** Releases all resources. */
    void cleanupResources();
    
    //======================================================================================
    static WindowManager* s_instance;
    
    bool m_isInitialized;
    bool m_isRunning;
    
    std::vector<std::unique_ptr<BaseWindow>> m_mainWindows;
    
    void* m_sharedRenderDevice;
    
    std::vector<Window*> m_allWindows;
    std::vector<std::unique_ptr<BaseWindow>> m_dialogs;
    std::vector<std::unique_ptr<BaseWindow>> m_toolWindows;
    
    WindowManager (const WindowManager&) = delete;
    WindowManager& operator= (const WindowManager&) = delete;
};

} // namespace YuchenUI
