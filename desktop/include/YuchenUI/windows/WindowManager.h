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
class IFontProvider;
class IThemeProvider;
class IResourceResolver;

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
    wm.setFontProvider(fontProvider);
    wm.setThemeProvider(themeProvider);
    wm.setResourceResolver(resourceResolver);
    
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
    /** Sets the font provider for all windows.
        
        This font provider will be injected into all newly created windows.
        Must be called after initialize() and before creating any windows.
        
        @param provider  Font provider interface (must not be null)
    */
    void setFontProvider(IFontProvider* provider);
    
    /** Returns the font provider for this manager. */
    IFontProvider* getFontProvider() const { return m_fontProvider; }
    
    /** Sets the theme provider for all windows.
        
        This theme provider will be injected into all newly created windows.
        Must be called after initialize() and before creating any windows.
        
        @param provider  Theme provider interface (must not be null)
    */
    void setThemeProvider(IThemeProvider* provider);
    
    /** Returns the theme provider for this manager. */
    IThemeProvider* getThemeProvider() const { return m_themeProvider; }
    
    /** Sets the resource resolver for all windows.
        
        This resource resolver will be injected into all newly created windows.
        Must be called after initialize() and before creating any windows.
        
        @param resolver  Resource resolver interface (must not be null)
    */
    void setResourceResolver(IResourceResolver* resolver);
    
    /** Returns the resource resolver for this manager. */
    IResourceResolver* getResourceResolver() const { return m_resourceResolver; }
    
    //======================================================================================
    /** Starts the application event loop.
        
        This method blocks until quit() is called or all main windows are closed.
    */
    void run();
    
    /** Requests the application to quit.
        
        The event loop will exit after processing pending events.
    */
    void quit();

    //==========================================================================================
    /** Creates a main application window with content.
        
        Creates a window with WindowType::Main. By default, Main windows affect
        application lifetime (app quits when no lifetime-affecting windows remain).
        
        To create a Main-type window that doesn't affect app exit:
        @code
        auto* window = wm.createMainWindow<Content>(...);
        window->setAffectsAppLifetime(false);
        @endcode
        
        Initialization order:
        1. Create window
        2. Set target FPS
        3. Inject resource resolver
        4. Inject font provider (initializes renderer)
        5. Inject theme provider (into UIContext)
        6. Set content (calls onCreate with initialized context)
        
        @tparam ContentType  The IUIContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param fps           Target frame rate (range: 15-240)
        @param args          Arguments forwarded to ContentType constructor
        
        @returns Pointer to the created window, or nullptr on failure
        
        @see setAffectsAppLifetime(), createDialog(), createToolWindow()
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createMainWindow(int width, int height, const char* title, int fps, Args&&... args)
    {
        if (!m_isInitialized)
        {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto mainWindow = std::make_unique<BaseWindow>(WindowType::Main);
        mainWindow->setTargetFPS(fps);
        
        if (!mainWindow->create(width, height, title, nullptr))
        {
            return nullptr;
        }
        
        BaseWindow* mainWindowPtr = mainWindow.get();
        
        if (m_resourceResolver)
        {
            mainWindowPtr->setResourceResolver(m_resourceResolver);
        }
        
        if (m_fontProvider)
        {
            mainWindowPtr->setFontProvider(m_fontProvider);
        }
        
        if (m_themeProvider)
        {
            mainWindowPtr->getUIContext().setThemeProvider(m_themeProvider);
        }
        
        auto content = std::make_unique<ContentType>(std::forward<Args>(args)...);
        mainWindowPtr->setContent(std::move(content));
        
        m_mainWindows.push_back(std::move(mainWindow));
        registerWindow(mainWindowPtr);
        
        return mainWindowPtr;
    }

    //==========================================================================================
    /** Creates a modal dialog window with content.
        
        Dialogs are temporary windows typically used for user interaction.
        
        Initialization order:
        1. Create window
        2. Set target FPS
        3. Inject resource resolver
        4. Inject font provider (initializes renderer)
        5. Inject theme provider (into UIContext)
        6. Set content (calls onCreate with initialized context)
        
        @tparam ContentType  The IUIContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param parent        Parent window for the dialog
        @param fps           Target frame rate (range: 15-240)
        @param args          Arguments forwarded to ContentType constructor
        
        @returns Pointer to the created dialog, or nullptr on failure
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createDialog(int width, int height, const char* title, Window* parent, int fps, Args&&... args)
    {
        if (!m_isInitialized)
        {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto dialog = std::make_unique<BaseWindow>(WindowType::Dialog);
        dialog->setTargetFPS(fps);
        
        if (!dialog->create(width, height, title, parent))
        {
            return nullptr;
        }
        
        BaseWindow* dialogPtr = dialog.get();
        
        if (m_resourceResolver)
        {
            dialogPtr->setResourceResolver(m_resourceResolver);
        }
        
        if (m_fontProvider)
        {
            dialogPtr->setFontProvider(m_fontProvider);
        }
        
        if (m_themeProvider)
        {
            dialogPtr->getUIContext().setThemeProvider(m_themeProvider);
        }
        
        auto content = std::make_unique<ContentType>(std::forward<Args>(args)...);
        dialogPtr->setContent(std::move(content));
        
        m_dialogs.push_back(std::move(dialog));
        registerWindow(dialogPtr);
        
        return dialogPtr;
    }

    //==========================================================================================
    /** Creates a tool window with content.
        
        Tool windows are typically used for auxiliary UI such as palettes or inspectors.
        
        Initialization order:
        1. Create window
        2. Set target FPS
        3. Inject resource resolver
        4. Inject font provider (initializes renderer)
        5. Inject theme provider (into UIContext)
        6. Set content (calls onCreate with initialized context)
        
        @tparam ContentType  The IUIContent-derived class to create
        @param width         Window width in pixels
        @param height        Window height in pixels
        @param title         Window title text
        @param parent        Parent window for the tool window
        @param fps           Target frame rate (range: 15-240)
        @param args          Arguments forwarded to ContentType constructor
        
        @returns Pointer to the created tool window, or nullptr on failure
    */
    template<typename ContentType, typename... Args>
    BaseWindow* createToolWindow(int width, int height, const char* title, Window* parent, int fps, Args&&... args)
    {
        if (!m_isInitialized)
        {
            std::cerr << "[WindowManager] Not initialized" << std::endl;
            return nullptr;
        }
        
        auto toolWindow = std::make_unique<BaseWindow>(WindowType::ToolWindow);
        toolWindow->setTargetFPS(fps);
        
        if (!toolWindow->create(width, height, title, parent))
        {
            return nullptr;
        }
        
        BaseWindow* toolWindowPtr = toolWindow.get();
        
        if (m_resourceResolver)
        {
            toolWindowPtr->setResourceResolver(m_resourceResolver);
        }
        
        if (m_fontProvider)
        {
            toolWindowPtr->setFontProvider(m_fontProvider);
        }
        
        if (m_themeProvider)
        {
            toolWindowPtr->getUIContext().setThemeProvider(m_themeProvider);
        }
        
        auto content = std::make_unique<ContentType>(std::forward<Args>(args)...);
        toolWindowPtr->setContent(std::move(content));
        
        m_toolWindows.push_back(std::move(toolWindow));
        registerWindow(toolWindowPtr);
        
        return toolWindowPtr;
    }
    
    //======================================================================================
    /** Closes any window (main, dialog, or tool window).
        
        This is the unified method for closing windows. It checks the window's
        affectsAppLifetime property to determine if app should quit.
        
        @param window  The window to close (must not be null)
    */
    void closeWindow(BaseWindow* window);
    
    /** Closes all windows (main, dialog, and tool windows). */
    void closeAllWindows();
    
    /** Returns the number of windows that affect application lifetime.
        @returns Count of windows with affectsAppLifetime = true
    */
    size_t getLifetimeAffectingWindowCount() const;
    
    //======================================================================================
    /** Returns the shared rendering device handle.
        
        All windows share a single rendering device for efficiency.
    */
    void* getSharedRenderDevice() const { return m_sharedRenderDevice; }
    
    /** Registers a window with the manager. */
    void registerWindow(Window* window);
    
    /** Unregisters a window from the manager. */
    void unregisterWindow(Window* window);
    
    /** Schedules a dialog for destruction after the current event loop iteration.
        
        This is used internally for modal dialogs that need to close themselves.
        
        @param dialog  The dialog to destroy
    */
    void scheduleDialogDestruction(BaseWindow* dialog);
    
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
    
    IFontProvider* m_fontProvider;
    IThemeProvider* m_themeProvider;
    IResourceResolver* m_resourceResolver;
    
    std::vector<std::unique_ptr<BaseWindow>> m_mainWindows;
    
    void* m_sharedRenderDevice;
    
    std::vector<Window*> m_allWindows;
    std::vector<std::unique_ptr<BaseWindow>> m_dialogs;
    std::vector<std::unique_ptr<BaseWindow>> m_toolWindows;
    
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
};

} // namespace YuchenUI
