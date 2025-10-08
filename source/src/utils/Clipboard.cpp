#include "YuchenUI/utils/Clipboard.h"

#ifdef __APPLE__
    #import <AppKit/AppKit.h>
#elif defined(_WIN32)
    #include <windows.h>
#endif

namespace YuchenUI {

void Clipboard::setText(const std::string& text) {
#ifdef __APPLE__
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        [pasteboard setString:nsText forType:NSPasteboardTypeString];
    }
#elif defined(_WIN32)
    if (!OpenClipboard(nullptr)) return;
    
    EmptyClipboard();
    
    size_t size = text.length() + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hMem) {
        memcpy(GlobalLock(hMem), text.c_str(), size);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
    }
    
    CloseClipboard();
#endif
}

std::string Clipboard::getText() {
#ifdef __APPLE__
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* nsText = [pasteboard stringForType:NSPasteboardTypeString];
        if (nsText) {
            return std::string([nsText UTF8String]);
        }
    }
#elif defined(_WIN32)
    if (!OpenClipboard(nullptr)) return "";
    
    std::string result;
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData) {
        char* pszText = static_cast<char*>(GlobalLock(hData));
        if (pszText) {
            result = pszText;
            GlobalUnlock(hData);
        }
    }
    
    CloseClipboard();
    return result;
#endif
    
    return "";
}

}
