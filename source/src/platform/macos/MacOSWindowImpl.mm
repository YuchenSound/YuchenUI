#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#include "YuchenUI/platform/WindowImpl.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "MacEventManager.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Colors.h"
#include <atomic>

namespace YuchenUI { class MacOSWindowImpl; }

@interface UniversalWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) YuchenUI::MacOSWindowImpl* windowImpl;
@property (nonatomic, assign) YuchenUI::WindowType windowType;
@end

@interface UniversalMetalView : NSView <CALayerDelegate, NSTextInputClient>
{
    CAMetalLayer* metalLayer;
    id<MTLDevice> metalDevice;
    CVDisplayLinkRef displayLink;
    std::atomic<bool> isRendering;
}
@property (nonatomic, assign) YuchenUI::MacOSWindowImpl* windowImpl;
@property (nonatomic, assign) YuchenUI::WindowType windowType;
@property (nonatomic, strong) NSTimer* modalRefreshTimer;
@property (nonatomic, strong) NSMutableAttributedString* markedText;
@property (nonatomic, strong) NSTextInputContext* textInputContext;
@property (nonatomic, assign) BOOL imeEnabled;
- (void)performRenderCallback;
- (void)setImeEnabled:(BOOL)enabled;
@end

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                   const CVTimeStamp* now,
                                   const CVTimeStamp* outputTime,
                                   CVOptionFlags flagsIn,
                                   CVOptionFlags* flagsOut,
                                   void* displayLinkContext)
{
    UniversalMetalView* view = (__bridge UniversalMetalView*)displayLinkContext;
    [view performRenderCallback];
    return kCVReturnSuccess;
}

namespace YuchenUI
{

class MacOSWindowImpl : public WindowImpl
{
public:
    MacOSWindowImpl();
    virtual ~MacOSWindowImpl();
    
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
    
    void onEvent(void* event);
    void onRender();
    void onResize(int width, int height);
    void onWindowClosing();
    void handleMarkedText(const char* text, int cursorPos, int selectionLength);
    void handleCommittedText(const char* text);
    void handleUnmarkText();

private:
    NSWindowStyleMask createStyleMask(const WindowConfig& config);
    void positionWindow(const WindowConfig& config);
    void cleanupDelegateAndView();

    NSWindow* m_window;
    UniversalMetalView* m_metalView;
    UniversalWindowDelegate* m_delegate;
    BaseWindow* m_baseWindow;
    WindowType m_type;
};

}

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

@implementation UniversalMetalView

- (instancetype)initWithFrame:(NSRect)frame device:(id<MTLDevice>)device windowType:(YuchenUI::WindowType)type
{
    self = [super initWithFrame:frame];
    if (self)
    {
        metalDevice = device;
        metalLayer = nil;
        displayLink = nil;
        isRendering.store(false);
        self.windowType = type;
        self.windowImpl = nullptr;
        self.modalRefreshTimer = nil;
        self.markedText = nil;
        self.textInputContext = [[NSTextInputContext alloc] initWithClient:self];
        self.imeEnabled = YES;
        
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
        self.layerContentsPlacement = NSViewLayerContentsPlacementScaleAxesIndependently;
        
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
    
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink, &DisplayLinkCallback, (__bridge void*)self);
    CVDisplayLinkStart(displayLink);
#pragma clang diagnostic pop
}

- (void)stopRenderLoop
{
    if (displayLink)
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        CVDisplayLinkStop(displayLink);
        CVDisplayLinkRelease(displayLink);
#pragma clang diagnostic pop
        displayLink = NULL;
    }
    
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
    
    self.modalRefreshTimer = [NSTimer timerWithTimeInterval:(1.0 / 60.0)
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
- (void)doCommandBySelector:(SEL)selector {}

- (void)keyDown:(NSEvent *)event
{
    [self forwardEvent:event];
    [self interpretKeyEvents:@[event]];
}

- (void)forwardEvent:(NSEvent*)event
{
    if (self.windowImpl) self.windowImpl->onEvent((__bridge void*)event);
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)canBecomeKeyView { return YES; }

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

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    @autoreleasepool {
        NSString* characters;
        if ([string isKindOfClass:[NSAttributedString class]]) {
            characters = [string string];
        } else {
            characters = (NSString*)string;
        }
        
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
    
    YuchenUI::Rect cursorRect = self.windowImpl->getInputMethodCursorWindowRect();
    
    if (cursorRect.width == 0.0f || cursorRect.height == 0.0f) {
        NSWindow* window = [self window];
        if (!window) {
            return NSZeroRect;
        }
        
        NSRect windowFrame = [window frame];
        NSRect contentRect = [window contentRectForFrameRect:windowFrame];
        
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
    
    NSRect windowFrame = [window frame];
    NSRect contentRect = [window contentRectForFrameRect:windowFrame];
    
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

namespace YuchenUI
{

MacOSWindowImpl::MacOSWindowImpl()
    : m_window(nil)
    , m_metalView(nil)
    , m_delegate(nil)
    , m_baseWindow(nullptr)
    , m_type(WindowType::Main)
{
}

MacOSWindowImpl::~MacOSWindowImpl()
{
    destroy();
}

bool MacOSWindowImpl::create(const WindowConfig& config)
{
    YUCHEN_ASSERT(config.width > 0 && config.height > 0);
    YUCHEN_ASSERT(config.title);
    
    m_type = config.type;
    
    @autoreleasepool
    {
        NSRect windowRect = NSMakeRect(0, 0, config.width, config.height);
        NSWindowStyleMask styleMask = createStyleMask(config);
        
        if (config.type == WindowType::ToolWindow)
        {
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
            m_window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:styleMask
                                                     backing:NSBackingStoreBuffered defer:NO];
        }
        
        YUCHEN_ASSERT(m_window);
        
        [m_window setReleasedWhenClosed:NO];
        
        NSString* nsTitle = [NSString stringWithUTF8String:config.title];
        [m_window setTitle:nsTitle];
        [m_window setAcceptsMouseMovedEvents:YES];
        
        positionWindow(config);
        
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        YUCHEN_ASSERT(device);
        
        NSRect contentRect = NSMakeRect(0, 0, config.width, config.height);
        m_metalView = [[UniversalMetalView alloc] initWithFrame:contentRect device:device windowType:m_type];
        YUCHEN_ASSERT(m_metalView);
        
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
        [m_metalView startModalRefresh];
        [m_window center];
        [m_window makeKeyAndOrderFront:nil];
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
        
        for (NSUInteger i = 0; i < length; i++) {
            unichar ch = [nsText characterAtIndex:i];
            
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
        Vec2 parentPos = config.parent->getWindowPosition();
        Vec2 parentSize = config.parent->getSize();
        NSPoint newOrigin = NSMakePoint(parentPos.x + parentSize.x + 20, parentPos.y);
        [m_window setFrameOrigin:newOrigin];
    }
    else if (m_type != WindowType::Main)
    {
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

WindowImpl* WindowImplFactory::create()
{
    return new MacOSWindowImpl();
}

}
