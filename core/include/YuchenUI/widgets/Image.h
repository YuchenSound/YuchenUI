/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file Image.h
    
    Image display component with optional multi-frame sprite sheet support.
    
    Displays images from embedded resources with various scaling modes. Supports
    both single images and sprite sheet frame selection for frame-based animation
    or state visualization.
    
    Single image mode (default):
    @code
    Image* logo = new Image(Rect(10, 10, 100, 50));
    logo->setResource("logo@2x.png");
    logo->setScaleMode(ScaleMode::Fill);
    @endcode
    
    Multi-frame sprite sheet mode:
    @code
    Image* knob = new Image(Rect(10, 10, 34, 36));
    knob->setResource("knobs@2x.png");
    knob->setFrameConfiguration(28, Image::FrameDirection::Vertical, Vec2(34, 36));
    knob->setCurrentFrame(14);  // Show middle frame
    @endcode
*/

#pragma once

#include "YuchenUI/widgets/Widget.h"
#include "YuchenUI/core/Types.h"
#include <string>

namespace YuchenUI {

class RenderList;

//==========================================================================================
/**
    Image display component with optional sprite sheet support.
    
    By default, displays a single image from an embedded resource. Can be configured
    to display individual frames from a horizontal or vertical sprite sheet.
    
    Features:
    - Multiple scale modes (Original, Stretch, Fill, NineSlice)
    - Nine-slice scaling for resizable borders
    - Optional multi-frame sprite sheet support
    - Automatic DPI scaling (@1x, @2x, @3x)
    
    @see ScaleMode, NineSliceMargins
*/
class Image : public Widget {
public:
    //======================================================================================
    /** Frame arrangement direction in sprite sheet. */
    enum class FrameDirection {
        Horizontal,  ///< Frames arranged left to right
        Vertical     ///< Frames arranged top to bottom
    };
    
    //======================================================================================
    /**
        Creates an image component.
        
        @param bounds  Component bounds in logical pixels
    */
    explicit Image(const Rect& bounds);
    
    /** Destructor. */
    virtual ~Image();
    
    //======================================================================================
    // Rendering and interaction
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override { return false; }
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override { return false; }
    
    //======================================================================================
    // Basic image configuration
    
    /**
        Sets the image resource.
        
        @param resourceIdentifier  Path to embedded image resource
    */
    void setResource(const char* resourceIdentifier);
    
    /** Returns the current resource identifier. */
    const std::string& getResource() const { return m_resourceIdentifier; }
    
    /**
        Sets the scale mode for rendering.
        
        @param mode  Scaling behavior (Original, Stretch, Fill, NineSlice)
    */
    void setScaleMode(ScaleMode mode);
    
    /** Returns the current scale mode. */
    ScaleMode getScaleMode() const { return m_scaleMode; }
    
    /**
        Sets nine-slice margins for scalable borders.
        
        Only used when scale mode is ScaleMode::NineSlice.
        
        @param left    Left margin in pixels
        @param top     Top margin in pixels
        @param right   Right margin in pixels
        @param bottom  Bottom margin in pixels
    */
    void setNineSliceMargins(float left, float top, float right, float bottom);
    
    /**
        Sets nine-slice margins from structure.
        
        @param margins  Nine-slice margin structure
    */
    void setNineSliceMargins(const NineSliceMargins& margins);
    
    /** Returns the current nine-slice margins. */
    const NineSliceMargins& getNineSliceMargins() const { return m_nineSliceMargins; }
    
    //======================================================================================
    // Multi-frame sprite sheet support
    
    /**
        Configures sprite sheet frame layout.
        
        Enables multi-frame mode. After calling this, use setCurrentFrame() to
        select which frame to display.
        
        @param frameCount   Total number of frames in sprite sheet
        @param direction    Frame arrangement direction (horizontal or vertical)
        @param frameSize    Size of single frame in logical pixels
    */
    void setFrameConfiguration(int frameCount, FrameDirection direction, const Vec2& frameSize);
    
    /**
        Sets the current frame to display.
        
        Only used in multi-frame mode. Frame index is clamped to valid range.
        
        @param frameIndex  Zero-based frame index [0, frameCount-1]
    */
    void setCurrentFrame(int frameIndex);
    
    /** Returns the current frame index. */
    int getCurrentFrame() const { return m_currentFrame; }
    
    /** Returns the total number of frames (1 for single image mode). */
    int getFrameCount() const { return m_frameCount; }
    
    /** Returns true if multi-frame mode is enabled. */
    bool isMultiFrame() const { return m_frameCount > 1; }
    
    //======================================================================================
    // Component interface
    
    /**
        Validates component configuration.
        
        @returns True if resource is set and configuration is valid
    */
    bool isValid() const;

private:
    /**
        Calculates source rectangle for rendering.
        
        In single image mode, returns empty Rect (use full texture).
        In multi-frame mode, returns the rectangle for current frame.
        
        @returns Source rectangle in logical pixel coordinates
    */
    Rect calculateSourceRect() const;
    
    std::string m_resourceIdentifier;   ///< Resource path
    ScaleMode m_scaleMode;              ///< Scaling mode
    NineSliceMargins m_nineSliceMargins; ///< Nine-slice margins
    
    // Multi-frame support (defaults indicate single image mode)
    int m_frameCount;                   ///< Total frames (1 = single image)
    FrameDirection m_direction;         ///< Frame arrangement direction
    Vec2 m_frameSize;                   ///< Single frame size (0,0 = use full texture)
    int m_currentFrame;                 ///< Current frame index [0, frameCount-1]
};

} // namespace YuchenUI
