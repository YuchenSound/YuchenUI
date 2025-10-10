#pragma once

#include <string>

namespace YuchenUI {

/**
 * System clipboard utilities.
 *
 * Provides cross-platform text clipboard access.
 */
class Clipboard {
public:
    /**
     * Sets text to the system clipboard.
     * @param text  Text to copy
     */
    static void setText(const std::string& text);
    
    /**
     * Gets text from the system clipboard.
     * @returns Clipboard text, or empty string if unavailable
     */
    static std::string getText();
};

}
