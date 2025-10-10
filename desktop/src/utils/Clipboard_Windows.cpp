/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Utilities module (Windows).
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Clipboard_Windows.cpp
    
    Windows clipboard implementation using Win32 API.
    
    This file provides the Windows-specific implementation of the cross-platform clipboard
    API. It uses the Win32 clipboard API to interact with the system clipboard.
    
    Key implementation details:
    - Uses OpenClipboard/CloseClipboard for clipboard access
    - Allocates global memory with GMEM_MOVEABLE flag for clipboard data
    - Uses CF_TEXT format for ANSI text (could be extended to CF_UNICODETEXT)
    - Proper cleanup with GlobalUnlock after GlobalLock operations
    - Returns empty string on any error condition
    
    Memory management:
    - GlobalAlloc creates moveable memory block
    - System takes ownership of memory after SetClipboardData (do not free)
    - GlobalLock/Unlock required for accessing memory contents
    - EmptyClipboard frees previous clipboard contents
    
    Thread safety:
    - Clipboard operations are process-wide and not inherently thread-safe
    - OpenClipboard can fail if another application has clipboard open
    - Should typically be called from main thread for reliability
    
    Error handling:
    - Silently fails and returns empty string on errors
    - No exceptions thrown to maintain simple API contract
    
    Future improvements:
    - Consider using CF_UNICODETEXT for better Unicode support
    - Add retry logic for OpenClipboard failures
    - Implement GetLastError() logging for debugging
*/

#include "YuchenUI/utils/Clipboard.h"

#ifdef _WIN32

#include <windows.h>
#include <cstring>

namespace YuchenUI {

//==========================================================================================
// Clipboard Text Operations

void Clipboard::setText(const std::string& text) {
    // Attempt to open the clipboard
    // Pass nullptr to associate with current task rather than specific window
    if (!OpenClipboard(nullptr)) {
        return;
    }
    
    // Clear any existing clipboard contents
    // This also frees the memory of the previous clipboard data
    EmptyClipboard();
    
    // Allocate global memory for the text data
    // GMEM_MOVEABLE allows Windows to relocate memory as needed
    size_t size = text.length() + 1; // +1 for null terminator
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    
    if (hMem) {
        // Lock the memory block to get a pointer to its data
        void* pMem = GlobalLock(hMem);
        
        // Copy the text into the global memory block
        memcpy(pMem, text.c_str(), size);
        
        // Unlock the memory block
        GlobalUnlock(hMem);
        
        // Transfer ownership of memory to the clipboard
        // After this call, do NOT free hMem - the system owns it
        SetClipboardData(CF_TEXT, hMem);
    }
    
    // Close the clipboard, making the data available to other applications
    CloseClipboard();
}

std::string Clipboard::getText() {
    // Attempt to open the clipboard
    if (!OpenClipboard(nullptr)) {
        return "";
    }
    
    std::string result;
    
    // Retrieve handle to clipboard data in text format
    HANDLE hData = GetClipboardData(CF_TEXT);
    
    if (hData) {
        // Lock the memory to access its contents
        char* pszText = static_cast<char*>(GlobalLock(hData));
        
        if (pszText) {
            // Copy the text to our result string
            result = pszText;
            
            // Unlock the memory
            GlobalUnlock(hData);
        }
    }
    
    // Close the clipboard
    CloseClipboard();
    
    return result;
}

} // namespace YuchenUI

#endif // _WIN32
