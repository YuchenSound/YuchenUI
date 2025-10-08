#pragma once

#include <string>

namespace YuchenUI {

class Clipboard {
public:
    static void setText(const std::string& text);
    static std::string getText();

private:
    Clipboard() = delete;
};

}
