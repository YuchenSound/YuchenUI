#pragma once

namespace YuchenUI {

class ITextInputHandler {
public:
    virtual ~ITextInputHandler() = default;
    
    virtual void enableTextInput() = 0;
    virtual void disableTextInput() = 0;
    virtual void setIMEEnabled(bool enabled) = 0;
};

}
