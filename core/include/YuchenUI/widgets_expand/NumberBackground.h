/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"

namespace YuchenUI {

class RenderList;

/**
    Decorative background frame for numeric displays.
    
    NumberBackground provides a themed decorative frame consisting of:
    - Nine-slice scaled background image
    - Tiled texture overlay for visual interest
    
    This is a pure visual component with no interactive behavior.
    Typically used as a container for read-only numeric displays
    like SpinBox or Label components.
    
    Visual layers (back to front):
    1. Background image (nine-slice scaled, theme-specific)
    2. Tiled texture overlay (2x2px stipple pattern, DPI-aware)
    3. Child components (user-provided, e.g., SpinBox)
    
    The background supports arbitrary sizing through nine-slice scaling
    while preserving corner aesthetics. The texture overlay adapts to
    the frame size through hardware-accelerated tiling.
    
    Default dimensions match the embedded background images (69x17px)
    for optimal appearance, but the frame scales gracefully to any size.
    
    Example usage:
    @code
    // Create background frame
    NumberBackground* frame = parent->addChild(new NumberBackground(Rect(10, 10, 69, 17)));
    
    // Add read-only numeric display
    SpinBox* display = frame->addChild(new SpinBox(Rect(0, 0, 69, 17)));
    display->setHasBackground(false);
    display->setReadOnly(true);
    display->setValue(42.5);
    display->setSuffix("dB");
    @endcode
    
    @see SpinBox, Frame
*/
class NumberBackground : public Widget {
public:
    /**
        Default frame dimensions in logical pixels.
        
        These dimensions match the embedded background images and provide
        optimal visual appearance without scaling.
    */
    static constexpr float DEFAULT_WIDTH = 69.0f;
    static constexpr float DEFAULT_HEIGHT = 17.0f;
    
    /**
        Constructs a number background frame.
        
        If bounds are zero-sized, default dimensions are used.
        The frame is created with theme default colors and no padding.
        
        @param bounds  Initial bounding rectangle (uses default size if zero-sized)
    */
    explicit NumberBackground(const Rect& bounds);
    
    /** Destructor. */
    virtual ~NumberBackground();
    
    //======================================================================================
    // UIComponent Interface Implementation
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    
    //======================================================================================
    // Validation
    
    /**
        Validates frame state.
        
        @return true if bounds are valid, false otherwise
    */
    bool isValid() const;

private:
    /**
        Background nine-slice margins.
        
        Defines the non-stretchable edge regions for nine-slice scaling.
    */
    static constexpr float BACKGROUND_NINE_SLICE_MARGIN = 5.0f;
    
    /**
        Texture overlay insets from frame edges.
        
        Creates a visual border effect by rendering the tiled texture
        within a smaller rectangular region.
    */
    static constexpr float TEXTURE_INSET_LEFT = 3.0f;
    static constexpr float TEXTURE_INSET_TOP = 2.0f;
    static constexpr float TEXTURE_INSET_RIGHT = 3.0f;
    static constexpr float TEXTURE_INSET_BOTTOM = 3.0f;
};

} // namespace YuchenUI
