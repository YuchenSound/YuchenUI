#include "YuchenUI/utils/Clipboard.h"

#ifdef _WIN32

#include <windows.h>
#include <cstring>

namespace YuchenUI {

void Clipboard::setText(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return;
    }
    
    EmptyClipboard();
    
    size_t size = text.length() + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hMem) {
        memcpy(GlobalLock(hMem), text.c_str(), size);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
    }
    
    CloseClipboard();
}

std::string Clipboard::getText() {
    if (!OpenClipboard(nullptr)) {
        return "";
    }
    
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
}

}

#endif // _WIN32
