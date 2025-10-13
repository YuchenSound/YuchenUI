/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Rendering module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include "YuchenUI/core/Types.h"
#include <vector>

namespace YuchenUI {

//==========================================================================================
/**
    Command list for recording rendering operations.
    
    RenderList accumulates rendering commands in a linear vector for efficient
    execution by the graphics backend. Commands are validated on insertion and
    support hierarchical clipping via push/pop operations.
    
    Version 2.0 Changes:
    - Added drawText() overload with FontFallbackChain support
    - Deprecated legacy drawText() with two font handles
    - Enhanced validation for new text rendering commands
    
    Version 2.1 Changes:
    - Added drawImageRegion() for sprite sheet rendering
    
    Key features:
    - Cache-friendly linear command storage
    - Validation on command insertion
    - Hierarchical clipping with stack tracking
    - Text length capping at Config::Text::MAX_LENGTH
    - Command count capping at Config::Rendering::MAX_COMMANDS_PER_LIST
    
    Usage:
    @code
    RenderList cmdList;
    cmdList.clear(backgroundColor);
    cmdList.fillRect(bounds, color, cornerRadius);
    
    // New API with font fallback
    FontFallbackChain chain = fontProvider->createDefaultFallbackChain();
    cmdList.drawText("Hello世界😊", position, chain, 14.0f, textColor);
    
    cmdList.render();
    @endcode
    
    @see RenderCommand, IGraphicsBackend
*/
class RenderList
{
public:
    RenderList();
    ~RenderList();
    
    //======================================================================================
    // Drawing Commands
    
    /**
        Clears the render target with specified color.
        
        @param color  Clear color (RGBA)
    */
    void clear(const Vec4& color);
    
    /**
        Fills a rectangle with solid color and optional rounded corners.
        
        @param rect          Rectangle bounds
        @param color         Fill color (RGBA)
        @param cornerRadius  Corner rounding (default: sharp corners)
    */
    void fillRect(const Rect& rect, const Vec4& color,
                  const CornerRadius& cornerRadius = CornerRadius());
    
    /**
        Draws a rectangle outline with optional rounded corners.
        
        @param rect          Rectangle bounds
        @param color         Border color (RGBA)
        @param borderWidth   Border thickness in pixels
        @param cornerRadius  Corner rounding (default: sharp corners)
    */
    void drawRect(const Rect& rect, const Vec4& color, float borderWidth,
                  const CornerRadius& cornerRadius = CornerRadius());
    
    //======================================================================================
    // Text Drawing (New API with Font Fallback)
    
    /**
        Draws text with font fallback chain support.
        
        This is the new recommended API for text rendering. It supports:
        - Per-character font selection from fallback chain
        - Proper emoji and symbol rendering
        - Mixed-script text (Western + CJK + Emoji)
        
        The fallback chain allows automatic font selection based on character
        availability. For example, if the primary font doesn't have an emoji,
        the system will try the next font in the chain.
        
        Example:
        @code
        // Build fallback chain: Arial -> PingFang -> Emoji
        FontFallbackChain chain = fontManager.createDefaultFallbackChain();
        
        // Render mixed text with automatic font selection
        cmdList.drawText("Hello世界😊", position, chain, 14.0f, textColor);
        @endcode
        
        @param text           UTF-8 text string
        @param position       Text baseline position
        @param fallbackChain  Font fallback chain
        @param fontSize       Font size in points
        @param color          Text color (RGBA)
    */
    void drawText(const char* text, const Vec2& position,
                  const FontFallbackChain& fallbackChain,
                  float fontSize, const Vec4& color);
    

    
    //======================================================================================
    // Image Drawing
    
    /**
        Draws an image from resource identifier.
        
        Draws the entire image resource to the destination rectangle.
        
        @param resourceIdentifier  Resource name or path
        @param destRect           Destination rectangle
        @param scaleMode          Scaling mode (default: Stretch)
        @param nineSlice          Nine-slice margins (for NineSlice mode)
    */
    void drawImage(const char* resourceIdentifier, const Rect& destRect,
                   ScaleMode scaleMode = ScaleMode::Stretch,
                   const NineSliceMargins& nineSlice = NineSliceMargins());
    
    /**
        Draws a region of an image (sprite sheet support).
        
        This overload allows rendering a portion of an image, useful for sprite sheets
        and multi-frame animations. The source rectangle specifies which region of the
        source texture to draw.
        
        Note: Nine-slice scaling is not supported when using explicit source rectangles.
        
        @param resourceIdentifier  Resource name or path
        @param destRect           Destination rectangle in logical pixels
        @param sourceRect         Source rectangle in logical pixels (region to sample)
        @param scaleMode          Scaling mode (default: Stretch, NineSlice not supported)
    */
    void drawImageRegion(const char* resourceIdentifier,
                         const Rect& destRect,
                         const Rect& sourceRect,
                         ScaleMode scaleMode = ScaleMode::Stretch);
    
    //======================================================================================
    // Shape Drawing
    
    /**
        Draws a line between two points.
        
        @param start  Start position
        @param end    End position
        @param color  Line color (RGBA)
        @param width  Line width in pixels (default: 1.0)
    */
    void drawLine(const Vec2& start, const Vec2& end, const Vec4& color,
                  float width = 1.0f);
    
    /**
        Fills a triangle with solid color.
        
        @param p1     First vertex
        @param p2     Second vertex
        @param p3     Third vertex
        @param color  Fill color (RGBA)
    */
    void fillTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3,
                     const Vec4& color);
    
    /**
        Draws a triangle outline.
        
        @param p1           First vertex
        @param p2           Second vertex
        @param p3           Third vertex
        @param color        Border color (RGBA)
        @param borderWidth  Border thickness in pixels (default: 1.0)
    */
    void drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3,
                     const Vec4& color, float borderWidth = 1.0f);
    
    /**
        Fills a circle with solid color.
        
        @param center  Circle center position
        @param radius  Circle radius in pixels
        @param color   Fill color (RGBA)
    */
    void fillCircle(const Vec2& center, float radius, const Vec4& color);
    
    /**
        Draws a circle outline.
        
        @param center       Circle center position
        @param radius       Circle radius in pixels
        @param color        Border color (RGBA)
        @param borderWidth  Border thickness in pixels (default: 1.0)
    */
    void drawCircle(const Vec2& center, float radius, const Vec4& color,
                    float borderWidth = 1.0f);

    //======================================================================================
    // Clipping
    
    /**
        Pushes a clipping rectangle onto the stack.
        
        All subsequent draw commands will be clipped to this rectangle
        until popClipRect() is called.
        
        @param rect  Clipping rectangle in window coordinates
    */
    void pushClipRect(const Rect& rect);
    
    /**
        Pops the top clipping rectangle from the stack.
        
        Restores clipping to the previous state.
    */
    void popClipRect();
    
    //======================================================================================
    // State Management
    
    /**
        Resets the command list, clearing all commands and clipping state.
    */
    void reset();
    
    /**
        Returns true if the command list is empty.
    */
    bool isEmpty() const;
    
    /**
        Returns the number of commands in the list.
    */
    size_t getCommandCount() const;
    
    /**
        Returns the internal command vector.
        
        For use by graphics backend during command execution.
        
        @returns Const reference to command vector
    */
    const std::vector<RenderCommand>& getCommands() const;
    
    /**
        Validates all commands in the list.
        
        Checks command parameters and invariants. Used for debugging.
        
        @returns True if all commands are valid
    */
    bool validate() const;
    
private:
    /**
        Adds a command to the list.
        
        @param cmd  Command to add
    */
    void addCommand(const RenderCommand& cmd);
    
    /**
        Validates a command before adding.
        
        @param cmd  Command to validate
    */
    void validateCommand(const RenderCommand& cmd) const;
    
    std::vector<RenderCommand> m_commands;  ///< Command buffer
    std::vector<Rect> m_clipStack;          ///< Clipping rectangle stack
};

} // namespace YuchenUI
