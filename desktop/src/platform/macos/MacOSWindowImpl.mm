/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Platform module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file MacOSWindowImpl.mm
    
    macOS-specific window implementation using Cocoa NSWindow and Metal rendering.
    
    Implementation notes:
    - Uses NSWindow for window management and NSView for content
    - Metal layer (CAMetalLayer) attached to view for GPU rendering
    - Display link (CVDisplayLink) drives frame rendering at configurable frame rate
    - Frame rate control implemented via frame skip logic in display link callback
    - Modal dialogs use NSTimer for rendering with same frame rate control
    - IME (Input Method Editor) support through NSTextInputClient protocol
    - Coordinate system converted from Cocoa (bottom-left) to YuchenUI (top-left)
    - Three window types supported: Main, Dialog, and ToolWindow (NSPanel)
    
    Frame rate control:
    - Each window has independent target FPS setting (15-240 fps)
    - CVDisplayLink runs at display refresh rate, frame skip logic limits actual rendering
    - Skip interval calculated as: displayRefreshRate / targetFPS
    - Example: 144Hz display with 60fps target = skip interval of 2.4, rounded to 2
    - Frame counter tracks when to render based on skip interval
*/

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "MacEventManager.h"
#include "YuchenUI/core/Assert.h"

#include <atomic>

namespace YuchenUI { class MacOSWindowImpl; }

//==========================================================================================
/**
    NSWindowDelegate implementation for window lifecycle events.
    
    Forwards window events (close, resize) to the MacOSWindowImpl.
*/
@interface UniversalWindowDelegate : NSObject <NSWindowDelegate>

/** Pointer to the C++ window implementation. */
@property (nonatomic, assign) YuchenUI::MacOSWindowImpl* windowImpl;

/** The type of window (Main, Dialog, or ToolWindow). */
@property (nonatomic, assign) YuchenUI::WindowType windowType;

@end

//==========================================================================================
/**
    Custom NSView with Metal rendering and text input support.
    
    This view:
    - Manages a CAMetalLayer for GPU rendering
    - Implements NSTextInputClient for IME support
    - Uses CVDisplayLink for synchronized frame updates with frame rate control
    - Forwards mouse and keyboard events to MacOSWindowImpl
    - Provides separate rendering path for modal dialogs using NSTimer
    - Implements frame skip logic to achieve target frame rate
*/
@interface UniversalMetalView : NSView <CALayerDelegate, NSTextInputClient>
{
    CAMetalLayer* metalLayer;           ///< Metal rendering layer
    id<MTLDevice> metalDevice;          ///< Metal device
    CVDisplayLinkRef displayLink;       ///< Display synchronization
    std::atomic<bool> isRendering;      ///< Prevents render reentrancy
}

/** Pointer to the C++ window implementation. */
@property (nonatomic, assign) YuchenUI::MacOSWindowImpl* windowImpl;

/** The window type. */
@property (nonatomic, assign) YuchenUI::WindowType windowType;

/** Timer for modal dialog frame updates. */
@property (nonatomic, strong) NSTimer* modalRefreshTimer;

/** Target frame rate for this window. */
@property (nonatomic, assign) int targetFPS;

/** Frame counter for skip logic. */
@property (nonatomic, assign) uint64_t frameCounter;

/** Cached display refresh rate. */
@property (nonatomic, assign) double displayRefreshRate;

/** Current IME composition text. */
@property (nonatomic, strong) NSMutableAttributedString* markedText;

/** Text input context for IME. */
@property (nonatomic, strong) NSTextInputContext* textInputContext;

/** Whether IME is enabled. */
@property (nonatomic, assign) BOOL imeEnabled;

/** Triggers a render callback on the main thread. */
- (void)performRenderCallback;

/** Enables or disables IME input.
    
    @param enabled  YES to enable, NO to disable
*/
- (void)setImeEnabled:(BOOL)enabled;

@end

//==========================================================================================
/**
    CVDisplayLink callback function.
    
    Called at display refresh rate. Implements frame skip logic to achieve target FPS.
    Schedules a render on the main thread to avoid threading issues with Metal.
    
    Frame skip algorithm:
    1. Get display refresh rate (e.g., 144 Hz)
    2. Calculate skip interval: displayRate / targetFPS
    3. Increment frame counter on each callback
    4. Render only when (frameCounter % skipInterval == 0)
    
    Example: 144 Hz display, 60 fps target
    - Skip interval = 144 / 60 = 2.4, rounded to 2
    - Renders on frames 0, 2, 4, 6, ... (approximately 72 fps)
    - Close enough to target, small error acceptable for UI
    
    @param displayLink       The display link
    @param now              Current time
    @param outputTime       Next output time
    @param flagsIn          Input flags
    @param flagsOut         Output flags
    @param displayLinkContext  User context (UniversalMetalView*)
    @returns kCVReturnSuccess
*/
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                   const CVTimeStamp* now,
                                   const CVTimeStamp* outputTime,
                                   CVOptionFlags flagsIn,
                                   CVOptionFlags* flagsOut,
                                   void* displayLinkContext)
{
    UniversalMetalView* view = (__bridge UniversalMetalView*)displayLinkContext;
    
    // Increment frame counter
    view.frameCounter++;
    
    // Get display refresh rate
    double displayRate = view.displayRefreshRate;
    if (displayRate == 0) {
        displayRate = 60.0;
    }
    
    // Get target FPS, validate range
    int targetFPS = view.targetFPS;
    if (targetFPS <= 0 || targetFPS > 240) {
        targetFPS = 60;
    }
    
    // If target FPS is higher than display rate, render every frame
    if (targetFPS >= displayRate) {
        [view performRenderCallback];
        return kCVReturnSuccess;
    }
    
    // Calculate skip interval
    uint64_t skipInterval = (uint64_t)(displayRate / targetFPS + 0.5);
    if (skipInterval < 1) {
        skipInterval = 1;
    }
    
    // Render only on skip interval frames
    if (view.frameCounter % skipInterval == 0) {
        [view performRenderCallback];
    }
    
    return kCVReturnSuccess;
}

namespace YuchenUI
{

//==========================================================================================
/**
    macOS implementation of WindowImpl using Cocoa and Metal.
    
    MacOSWindowImpl manages the native NSWindow and its Metal-backed content view.
    It handles event forwarding, rendering coordination, IME input, and frame rate control.
    
    @see WindowImpl, BaseWindow
*/
class MacOSWindowImpl : public WindowImpl
{
public:
    /** Creates a MacOSWindowImpl instance. */
    MacOSWindowImpl();
    
    /** Destructor. Cleans up window resources. */
    virtual ~MacOSWindowImpl();
    
    //======================================================================================
    // WindowImpl Interface Implementation
    
    /** Creates the native window and view.
        
        @param config  Window configuration parameters including target FPS
        @returns True if creation succeeded, false otherwise
    */
    bool create(const WindowConfig& config) override;
    
    /** Destroys the native window and releases resources. */
    void destroy() override;
    
    /** Shows the window. */
    void show() override;
    
    /** Hides the window. */
    void hide() override;
    
    /** Displays the window as a modal dialog (blocks until closed). */
    void showModal() override;
    
    /** Closes a modal dialog and exits the modal loop. */
    void closeModal() override;
    
    /** Returns the current window content size.
        
        @returns Size in logical pixels (not physical pixels)
    */
    Vec2 getSize() const override;
    
    /** Returns the window's position on screen. */
    Vec2 getPosition() const override;
    
    /** Returns true if the window is currently visible. */
    bool isVisible() const override;
    
    /** Returns the native window handle (NSWindow*). */
    void* getNativeHandle() const override;
    
    //======================================================================================
    /** Sets the associated BaseWindow for callbacks.
        
        @param baseWindow  The BaseWindow that owns this implementation
    */
    void setBaseWindow(BaseWindow* baseWindow) override;
    
    /** Returns the Metal render surface (CAMetalLayer*). */
    void* getRenderSurface() const override;
    
    /** Returns the display's DPI scale factor. */
    float getDpiScale() const override;
    
    /** Converts window coordinates to screen coordinates.
        
        @param windowPos  Position in window space
        @returns Position in screen space
    */
    Vec2 mapToScreen(const Vec2& windowPos) const override;
    
    /** Returns the IME cursor rectangle in window coordinates. */
    Rect getInputMethodCursorWindowRect() const override;
    
    /** Enables or disables IME input.
        
        @param enabled  True to enable, false to disable
    */
    void setIMEEnabled(bool enabled) override;
    
    //======================================================================================
    /** Forwards a native event to the BaseWindow.
        
        @param event  The native NSEvent*
    */
    void onEvent(void* event);
    
    /** Triggers a render frame. */
    void onRender();
    
    /** Handles window resize events.
        
        @param width   New width in pixels
        @param height  New height in pixels
    */
    void onResize(int width, int height);
    
    /** Handles window close requests. */
    void onWindowClosing();
    
    /** Handles IME marked text.
        
        @param text              The marked text
        @param cursorPos         Cursor position
        @param selectionLength   Selection length
    */
    void handleMarkedText(const char* text, int cursorPos, int selectionLength);
    
    /** Handles committed text from IME.
        
        @param text  The committed text string
    */
    void handleCommittedText(const char* text);
    
    /** Handles IME unmark (composition completion). */
    void handleUnmarkText();

private:
    //======================================================================================
    /** Creates the NSWindow style mask based on configuration.
        
        @param config  Window configuration
        @returns The appropriate NSWindowStyleMask
    */
    NSWindowStyleMask createStyleMask(const WindowConfig& config);
    
    /** Positions the window on screen.
        
        @param config  Window configuration with parent info
    */
    void positionWindow(const WindowConfig& config);
    
    /** Cleans up delegate and view, breaking retain cycles. */
    void cleanupDelegateAndView();

    //======================================================================================
    NSWindow* m_window;                     ///< The native NSWindow
    UniversalMetalView* m_metalView;        ///< The Metal-backed content view
    UniversalWindowDelegate* m_delegate;    ///< Window delegate for events
    BaseWindow* m_baseWindow;               ///< Associated YuchenUI window
    WindowType m_type;                      ///< Window type
    int m_targetFPS;                        ///< Target frame rate for this window
};

} // namespace YuchenUI

//==========================================================================================
// UniversalWindowDelegate Implementation

@implementation UniversalWindowDelegate

- (BOOL)windowShouldClose:(NSWindow *)window
{
    if (self.windowImpl) {
        self.windowImpl->onWindowClosing();
    }
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification
{
    NSWindow* window = notification.object;
    [window setDelegate:nil];
    
    if (self.windowImpl) {
        self.windowImpl = nullptr;
    }
}

- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow* window = notification.object;
    NSRect frame = [window contentRectForFrameRect:window.frame];
    if (self.windowImpl) {
        self.windowImpl->onResize((int)frame.size.width, (int)frame.size.height);
    }
}

@end

//==========================================================================================
// UniversalMetalView Implementation

@implementation UniversalMetalView

- (instancetype)initWithFrame:(NSRect)frame device:(id<MTLDevice>)device windowType:(YuchenUI::WindowType)type targetFPS:(int)fps
{
    self = [super initWithFrame:frame];
    if (self)
    {
        metalDevice = device;
        metalLayer = nil;
        displayLink = nil;
        isRendering.store(false);
        self.windowType = type;
        self.targetFPS = fps;
        self.frameCounter = 0;
        self.displayRefreshRate = 0;
        self.windowImpl = nullptr;
        self.modalRefreshTimer = nil;
        self.markedText = nil;
        self.textInputContext = [[NSTextInputContext alloc] initWithClient:self];
        self.imeEnabled = YES;
        
        // Configure view for layer backing
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
        self.layerContentsPlacement = NSViewLayerContentsPlacementScaleAxesIndependently;
        
        // Set up mouse tracking
        NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingActiveInActiveApp | NSTrackingInVisibleRect;
        NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:frame options:options owner:self userInfo:nil];
        [self addTrackingArea:trackingArea];
        
        [self startRenderLoop];
    }
    return self;
}

- (void)dealloc
{
    [self stopRenderLoop];
}

- (CALayer *)makeBackingLayer
{
    // Create CAMetalLayer for GPU rendering
    metalLayer = [CAMetalLayer layer];
    metalLayer.device = metalDevice;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.delegate = self;
    metalLayer.allowsNextDrawableTimeout = NO;
    metalLayer.autoresizingMask = kCALayerHeightSizable | kCALayerWidthSizable;
    metalLayer.needsDisplayOnBoundsChange = YES;
    metalLayer.presentsWithTransaction = YES;
    metalLayer.opaque = YES;
    
    NSSize frameSize = self.frame.size;
    CGSize drawableSize = [self convertSizeToBacking:frameSize];
    metalLayer.drawableSize = drawableSize;
    
    return metalLayer;
}

- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    
    if (metalLayer)
    {
        CGSize drawableSize = [self convertSizeToBacking:newSize];
        metalLayer.drawableSize = drawableSize;
    }
    
    if (self.windowImpl)
    {
        self.windowImpl->onResize((int)newSize.width, (int)newSize.height);
    }
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    if (!self.window) return;
    
    if (metalLayer)
    {
        CGFloat scaleFactor = self.window.backingScaleFactor;
        metalLayer.contentsScale = scaleFactor;
    }
}

- (void)displayLayer:(CALayer *)layer
{
    // Prevent reentrancy
    if (isRendering.load()) return;
    
    isRendering.store(true);
    
    @autoreleasepool {
        if (self.windowImpl)
        {
            self.windowImpl->onRender();
        }
    }
    
    isRendering.store(false);
}

- (void)startRenderLoop
{
    if (displayLink) return;
    
    // Create display link
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    
    // Get the display ID for this window
    CGDirectDisplayID displayID = (CGDirectDisplayID)[[[self.window screen] deviceDescription][@"NSScreenNumber"] unsignedIntValue];
    CVDisplayLinkSetCurrentCGDisplay(displayLink, displayID);
    
    // Query display refresh rate
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(displayID);
    if (mode) {
        self.displayRefreshRate = CGDisplayModeGetRefreshRate(mode);
        CGDisplayModeRelease(mode);
        
        // Some displays return 0 for refresh rate, default to 60
        if (self.displayRefreshRate == 0) {
            self.displayRefreshRate = 60.0;
        }
    } else {
        self.displayRefreshRate = 60.0;
    }
    
    // Set callback and start
    CVDisplayLinkSetOutputCallback(displayLink, &DisplayLinkCallback, (__bridge void*)self);
    CVDisplayLinkStart(displayLink);
}

- (void)stopRenderLoop
{
    if (displayLink)
    {
        CVDisplayLinkStop(displayLink);
        CVDisplayLinkRelease(displayLink);
        displayLink = NULL;
    }
    
    // Wait for any in-progress render to complete
    int waitCount = 0;
    while (isRendering.load() && waitCount < 1000)
    {
        usleep(100);
        waitCount++;
    }
}

- (void)performRenderCallback
{
    if (isRendering.load()) return;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self->metalLayer)
        {
            [self->metalLayer setNeedsDisplay];
        }
    });
}

- (void)startModalRefresh
{
    if (self.modalRefreshTimer) return;
    
    // Create timer for modal dialog rendering using target FPS
    double interval = 1.0 / (double)self.targetFPS;
    
    self.modalRefreshTimer = [NSTimer timerWithTimeInterval:interval
                                                     target:self
                                                   selector:@selector(modalRefreshTick:)
                                                   userInfo:nil
                                                    repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:self.modalRefreshTimer forMode:NSModalPanelRunLoopMode];
    [[NSRunLoop currentRunLoop] addTimer:self.modalRefreshTimer forMode:NSRunLoopCommonModes];
}

- (void)stopModalRefresh
{
    if (self.modalRefreshTimer)
    {
        [self.modalRefreshTimer invalidate];
        self.modalRefreshTimer = nil;
    }
}

- (void)modalRefreshTick:(NSTimer*)timer
{
    [metalLayer setNeedsDisplay];
}

//==========================================================================================
// Event Forwarding

- (void)mouseDown:(NSEvent *)event { [self forwardEvent:event]; }
- (void)mouseUp:(NSEvent *)event { [self forwardEvent:event]; }
- (void)rightMouseDown:(NSEvent *)event { [self forwardEvent:event]; }
- (void)rightMouseUp:(NSEvent *)event { [self forwardEvent:event]; }
- (void)otherMouseDown:(NSEvent *)event { [self forwardEvent:event]; }
- (void)otherMouseUp:(NSEvent *)event { [self forwardEvent:event]; }
- (void)mouseMoved:(NSEvent *)event { [self forwardEvent:event]; }
- (void)mouseDragged:(NSEvent *)event { [self forwardEvent:event]; }
- (void)rightMouseDragged:(NSEvent *)event { [self forwardEvent:event]; }
- (void)otherMouseDragged:(NSEvent *)event { [self forwardEvent:event]; }
- (void)scrollWheel:(NSEvent *)event { [self forwardEvent:event]; }
- (void)keyUp:(NSEvent *)event { [self forwardEvent:event]; }
- (void)flagsChanged:(NSEvent *)event { [self forwardEvent:event]; }

/** Empty implementation to prevent system beep on unhandled commands. */
- (void)doCommandBySelector:(SEL)selector {}

- (void)keyDown:(NSEvent *)event
{
    [self forwardEvent:event];
    // Allow text input system to interpret the key
    [self interpretKeyEvents:@[event]];
}

- (void)forwardEvent:(NSEvent*)event
{
    if (self.windowImpl) self.windowImpl->onEvent((__bridge void*)event);
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)canBecomeKeyView { return YES; }

//==========================================================================================
// Text Input Management

- (NSTextInputContext *)inputContext
{
    return self.textInputContext;
}

- (BOOL)becomeFirstResponder
{
    BOOL result = [super becomeFirstResponder];
    if (result) {
        [self.textInputContext activate];
    }
    return result;
}

- (BOOL)resignFirstResponder
{
    [self.textInputContext deactivate];
    return [super resignFirstResponder];
}

//==========================================================================================
// NSTextInputClient Protocol Implementation

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    @autoreleasepool {
        NSString* characters;
        if ([string isKindOfClass:[NSAttributedString class]]) {
            characters = [string string];
        } else {
            characters = (NSString*)string;
        }
        
        // Clear any marked text
        [self unmarkText];
        
        if (!self.windowImpl || [characters length] == 0) return;
        
        const char* utf8Text = [characters UTF8String];
        if (utf8Text && utf8Text[0] != '\0') {
            self.windowImpl->handleCommittedText(utf8Text);
        }
    }
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    // If IME is disabled, reject marked text
    if (!self.imeEnabled) {
        [self unmarkText];
        return;
    }
    
    @autoreleasepool {
        if ([string isKindOfClass:[NSAttributedString class]]) {
            self.markedText = [[NSMutableAttributedString alloc] initWithAttributedString:string];
        } else {
            self.markedText = [[NSMutableAttributedString alloc] initWithString:string];
        }
        
        if (self.windowImpl) {
            NSString* markedString = [self.markedText string];
            const char* utf8Text = [markedString UTF8String];
            
            YuchenUI::MacOSWindowImpl* impl = self.windowImpl;
            impl->handleMarkedText(utf8Text, (int)selectedRange.location, (int)selectedRange.length);
        }
    }
}

- (void)unmarkText
{
    @autoreleasepool {
        if (self.markedText) {
            self.markedText = nil;
            
            if (self.windowImpl) {
                YuchenUI::MacOSWindowImpl* impl = self.windowImpl;
                impl->handleUnmarkText();
            }
        }
    }
}

- (BOOL)hasMarkedText
{
    return self.markedText != nil && [[self.markedText string] length] > 0;
}

- (NSRange)markedRange
{
    if (self.markedText) {
        return NSMakeRange(0, [[self.markedText string] length]);
    }
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    return nil;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText
{
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    if (!self.windowImpl) {
        return NSZeroRect;
    }
    
    // Get cursor position from window
    YuchenUI::Rect cursorRect = self.windowImpl->getInputMethodCursorWindowRect();
    
    // Return default position if no cursor rect available
    if (cursorRect.width == 0.0f || cursorRect.height == 0.0f) {
        NSWindow* window = [self window];
        if (!window) {
            return NSZeroRect;
        }
        
        NSRect windowFrame = [window frame];
        NSRect contentRect = [window contentRectForFrameRect:windowFrame];
        
        // Return center of window as default
        return NSMakeRect(
            contentRect.origin.x + contentRect.size.width / 2,
            contentRect.origin.y + contentRect.size.height / 2,
            1.0,
            20.0
        );
    }
    
    NSWindow* window = [self window];
    if (!window) {
        return NSZeroRect;
    }
    
    // Convert window coordinates to screen coordinates
    NSRect windowFrame = [window frame];
    NSRect contentRect = [window contentRectForFrameRect:windowFrame];
    
    // Convert from top-left origin (YuchenUI) to bottom-left origin (Cocoa)
    float screenTopY = contentRect.origin.y + contentRect.size.height - cursorRect.y;
    float screenBottomY = screenTopY - cursorRect.height;
    float screenX = contentRect.origin.x + cursorRect.x;
    
    return NSMakeRect(
        screenX,
        screenBottomY,
        cursorRect.width,
        cursorRect.height
    );
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    return NSNotFound;
}

- (void)setImeEnabled:(BOOL)enabled
{
    _imeEnabled = enabled;
    
    if (!enabled) {
        [self unmarkText];
    }
}

@end

//==========================================================================================
// MacOSWindowImpl Implementation
//==========================================================================================

namespace YuchenUI
{

//==========================================================================================
// Construction and Destruction

MacOSWindowImpl::MacOSWindowImpl()
    : m_window(nil)
    , m_metalView(nil)
    , m_delegate(nil)
    , m_baseWindow(nullptr)
    , m_type(WindowType::Main)
    , m_targetFPS(Config::Rendering::DEFAULT_FPS)
{
}

MacOSWindowImpl::~MacOSWindowImpl()
{
    destroy();
}

//==========================================================================================
// Window Lifecycle

bool MacOSWindowImpl::create(const WindowConfig& config)
{
    YUCHEN_ASSERT(config.width > 0 && config.height > 0);
    YUCHEN_ASSERT(config.title);
    
    m_type = config.type;
    m_targetFPS = config.targetFPS;
    
    @autoreleasepool
    {
        NSRect windowRect = NSMakeRect(0, 0, config.width, config.height);
        NSWindowStyleMask styleMask = createStyleMask(config);
        
        // Create appropriate window type
        if (config.type == WindowType::ToolWindow)
        {
            // Use NSPanel for tool windows
            m_window = [[NSPanel alloc] initWithContentRect:windowRect styleMask:styleMask
                                                    backing:NSBackingStoreBuffered defer:NO];
            NSPanel* panel = (NSPanel*)m_window;
            [panel setFloatingPanel:config.floating];
            [panel setBecomesKeyOnlyIfNeeded:YES];
            [panel setHidesOnDeactivate:NO];
        }
        else if (config.type == WindowType::Dialog)
        {
            m_window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:styleMask
                                                     backing:NSBackingStoreBuffered defer:NO];
        }
        else
        {
            // Main window
            m_window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:styleMask
                                                     backing:NSBackingStoreBuffered defer:NO];
        }
        
        YUCHEN_ASSERT(m_window);
        
        [m_window setReleasedWhenClosed:NO];
        
        NSString* nsTitle = [NSString stringWithUTF8String:config.title];
        [m_window setTitle:nsTitle];
        [m_window setAcceptsMouseMovedEvents:YES];
        
        positionWindow(config);
        
        // Create Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        YUCHEN_ASSERT(device);
        
        // Create Metal view with target FPS
        NSRect contentRect = NSMakeRect(0, 0, config.width, config.height);
        m_metalView = [[UniversalMetalView alloc] initWithFrame:contentRect
                                                         device:device
                                                     windowType:m_type
                                                      targetFPS:m_targetFPS];
        YUCHEN_ASSERT(m_metalView);
        
        // Create window delegate
        m_delegate = [[UniversalWindowDelegate alloc] init];
        m_delegate.windowImpl = this;
        m_delegate.windowType = m_type;
        
        [m_window setDelegate:m_delegate];
        [m_window setContentView:m_metalView];
        
        m_metalView.windowImpl = this;
        
        return true;
    }
}

void MacOSWindowImpl::destroy()
{
    @autoreleasepool
    {
        // Stop modal refresh if active
        if (m_metalView) [m_metalView stopModalRefresh];
        
        cleanupDelegateAndView();
        
        if (m_window)
        {
            [m_window orderOut:nil];
            [m_window close];
            m_window = nil;
        }
    }
}

void MacOSWindowImpl::cleanupDelegateAndView()
{
    @autoreleasepool
    {
        if (m_delegate)
        {
            m_delegate.windowImpl = nullptr;
            m_delegate = nil;
        }
        
        if (m_metalView)
        {
            m_metalView.windowImpl = nullptr;
            // Clean up tracking areas
            NSArray* trackingAreas = [m_metalView.trackingAreas copy];
            for (NSTrackingArea* area in trackingAreas)
            {
                [m_metalView removeTrackingArea:area];
            }
            [m_metalView removeFromSuperview];
            m_metalView = nil;
        }
    }
}

//==========================================================================================
// Window Display

void MacOSWindowImpl::show()
{
    YUCHEN_ASSERT(m_window);
    @autoreleasepool
    {
        [m_window makeKeyAndOrderFront:nil];
        if (m_type != WindowType::ToolWindow) [m_window center];
    }
}

void MacOSWindowImpl::hide()
{
    YUCHEN_ASSERT(m_window);
    @autoreleasepool
    {
        [m_window orderOut:nil];
    }
}

void MacOSWindowImpl::showModal()
{
    YUCHEN_ASSERT(m_type == WindowType::Dialog);
    YUCHEN_ASSERT(m_window);
    
    @autoreleasepool
    {
        // Start timer-based refresh for modal dialogs
        [m_metalView startModalRefresh];
        [m_window center];
        [m_window makeKeyAndOrderFront:nil];
        
        // Block until modal is closed
        [NSApp runModalForWindow:m_window];
        
        [m_metalView stopModalRefresh];
        [m_window orderOut:nil];
    }
}

void MacOSWindowImpl::closeModal()
{
    YUCHEN_ASSERT(m_window);
    
    @autoreleasepool
    {
        if ([NSApp modalWindow] == m_window)
        {
            [NSApp stopModal];
        }
    }
}

//==========================================================================================
// Window Properties

Vec2 MacOSWindowImpl::getSize() const
{
    if (!m_window) return Vec2();
    NSRect frame = [m_window contentRectForFrameRect:[m_window frame]];
    return Vec2(frame.size.width, frame.size.height);
}

Vec2 MacOSWindowImpl::getPosition() const
{
    if (!m_window) return Vec2();
    NSRect frame = [m_window frame];
    return Vec2(frame.origin.x, frame.origin.y);
}

bool MacOSWindowImpl::isVisible() const
{
    return m_window && [m_window isVisible];
}

void* MacOSWindowImpl::getNativeHandle() const
{
    return (__bridge void*)m_window;
}

void MacOSWindowImpl::setBaseWindow(BaseWindow* baseWindow)
{
    m_baseWindow = baseWindow;
}

void* MacOSWindowImpl::getRenderSurface() const
{
    return (__bridge void*)m_metalView.layer;
}

float MacOSWindowImpl::getDpiScale() const
{
    if (!m_window) return 1.0f;
    
    @autoreleasepool {
        NSScreen* screen = [m_window screen];
        if (!screen) {
            screen = [NSScreen mainScreen];
        }
        return screen ? screen.backingScaleFactor : 1.0f;
    }
}

Vec2 MacOSWindowImpl::mapToScreen(const Vec2& windowPos) const
{
    if (!m_window) return Vec2();
    
    @autoreleasepool {
        NSRect windowFrame = [m_window frame];
        NSRect contentRect = [m_window contentRectForFrameRect:windowFrame];
        
        // Convert from top-left origin (YuchenUI) to screen coordinates
        float screenX = contentRect.origin.x + windowPos.x;
        float screenY = contentRect.origin.y + contentRect.size.height - windowPos.y;
        
        return Vec2(screenX, screenY);
    }
}

Rect MacOSWindowImpl::getInputMethodCursorWindowRect() const
{
    if (!m_baseWindow) return Rect();
    return m_baseWindow->getInputMethodCursorRect();
}

//==========================================================================================
// Event Handling

void MacOSWindowImpl::onEvent(void* event)
{
    if (m_baseWindow) m_baseWindow->handleNativeEvent(event);
}

void MacOSWindowImpl::onRender()
{
    if (m_baseWindow) m_baseWindow->renderContent();
}

void MacOSWindowImpl::onResize(int width, int height)
{
    if (m_baseWindow) m_baseWindow->onResize(width, height);
}

void MacOSWindowImpl::onWindowClosing()
{
    if (m_baseWindow) m_baseWindow->closeWithResult(WindowContentResult::Close);
}

//==========================================================================================
// IME Support

void MacOSWindowImpl::handleMarkedText(const char* text, int cursorPos, int selectionLength)
{
    if (m_baseWindow) {
        m_baseWindow->handleMarkedText(text, cursorPos, selectionLength);
    }
}

void MacOSWindowImpl::handleCommittedText(const char* text)
{
    if (!text || !text[0] || !m_baseWindow) return;
    
    @autoreleasepool {
        NSString* nsText = [NSString stringWithUTF8String:text];
        NSUInteger length = [nsText length];
        
        // Convert each character to a text input event
        for (NSUInteger i = 0; i < length; i++) {
            unichar ch = [nsText characterAtIndex:i];
            
            // Skip control characters
            if (ch < 32 || ch == 127) continue;
            
            double timestamp = CACurrentMediaTime();
            YuchenUI::Event textEvent = YuchenUI::Event::createTextInputEvent(ch, timestamp);
            
            m_baseWindow->handleEvent(textEvent);
        }
    }
}

void MacOSWindowImpl::handleUnmarkText()
{
    if (m_baseWindow) {
        m_baseWindow->handleUnmarkText();
    }
}

//==========================================================================================
// Private Methods

NSWindowStyleMask MacOSWindowImpl::createStyleMask(const WindowConfig& config)
{
    switch (config.type)
    {
        case WindowType::Main:
            return NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                   NSWindowStyleMaskMiniaturizable | (config.resizable ? NSWindowStyleMaskResizable : 0);
        case WindowType::Dialog:
            return NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
        case WindowType::ToolWindow:
            return NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskUtilityWindow;
    }
}

void MacOSWindowImpl::positionWindow(const WindowConfig& config)
{
    if (config.parent)
    {
        // Position relative to parent
        Vec2 parentPos = config.parent->getWindowPosition();
        Vec2 parentSize = config.parent->getSize();
        NSPoint newOrigin = NSMakePoint(parentPos.x + parentSize.x + 20, parentPos.y);
        [m_window setFrameOrigin:newOrigin];
    }
    else if (m_type != WindowType::Main)
    {
        // Center dialogs and tool windows
        [m_window center];
    }
}

void MacOSWindowImpl::setIMEEnabled(bool enabled)
{
    if (!m_metalView) return;
    
    @autoreleasepool {
        [m_metalView setImeEnabled:enabled ? YES : NO];
    }
}

//==========================================================================================
// Factory

WindowImpl* WindowImplFactory::create()
{
    return new MacOSWindowImpl();
}

} // namespace YuchenUI
