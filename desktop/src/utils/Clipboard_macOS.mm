/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Utilities module (macOS).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Clipboard_macOS.mm
    
    macOS clipboard implementation using NSPasteboard.
    
    This file provides the macOS-specific implementation of the cross-platform clipboard
    API. It uses the AppKit NSPasteboard API to interact with the system clipboard.
    
    Key implementation details:
    - Uses NSPasteboard generalPasteboard for system clipboard access
    - Wraps all Objective-C code in @autoreleasepool for proper memory management
    - Uses NSPasteboardTypeString (UTI: public.utf8-plain-text) for text data
    - Clears existing clipboard contents before setting new text
    - Returns empty string when clipboard contains no text data
    
    Thread safety:
    - NSPasteboard is thread-safe and can be accessed from any thread
    - However, clipboard operations should typically occur on main thread for consistency
    
    Character encoding:
    - All text handled as UTF-8 through NSString's UTF8String conversion
    - Automatically handles Unicode and emoji correctly
*/

#include "YuchenUI/utils/Clipboard.h"

#ifdef __APPLE__

#import <AppKit/AppKit.h>

namespace YuchenUI {

//==========================================================================================
// Clipboard Text Operations

void Clipboard::setText(const std::string& text) {
    @autoreleasepool {
        // Get the system clipboard
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        
        // Clear any existing clipboard contents
        [pasteboard clearContents];
        
        // Convert C++ string to NSString with UTF-8 encoding
        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        
        // Write the string to the clipboard as plain text
        // NSPasteboardTypeString corresponds to UTI: public.utf8-plain-text
        [pasteboard setString:nsText forType:NSPasteboardTypeString];
    }
}

std::string Clipboard::getText() {
    @autoreleasepool {
        // Get the system clipboard
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        
        // Attempt to read string data from clipboard
        NSString* nsText = [pasteboard stringForType:NSPasteboardTypeString];
        
        // Convert to C++ string if data exists
        if (nsText) {
            return std::string([nsText UTF8String]);
        }
    }
    
    // Return empty string if clipboard contains no text
    return "";
}

} // namespace YuchenUI

#endif // __APPLE__
