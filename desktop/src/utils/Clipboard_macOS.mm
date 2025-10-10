#include "YuchenUI/utils/Clipboard.h"

#ifdef __APPLE__

#import <AppKit/AppKit.h>

namespace YuchenUI {

void Clipboard::setText(const std::string& text) {
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        [pasteboard setString:nsText forType:NSPasteboardTypeString];
    }
}

std::string Clipboard::getText() {
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* nsText = [pasteboard stringForType:NSPasteboardTypeString];
        if (nsText) {
            return std::string([nsText UTF8String]);
        }
    }
    return "";
}

}

#endif // __APPLE__
