# YuchenUI

[![License](https://img.shields.io/badge/license-Unknown-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows-lightgrey.svg)]()
[![C++](https://img.shields.io/badge/C++-11%2B-orange.svg)]()

A lightweight, cross-platform C++ GUI framework with modern rendering capabilities.

## Overview

YuchenUI is a native GUI framework designed for building desktop applications with a focus on text rendering quality, cross-platform compatibility, and Pro Tools-inspired aesthetics. It provides a complete windowing system, widget library, and rendering pipeline.

## Features

### Cross-Platform Support

- **macOS**: Native Metal renderer
- **Windows**: Direct3D 11 renderer
- Shared rendering device architecture for optimal performance

### Text Rendering

- High-quality text rendering using FreeType and HarfBuzz
- Mixed script support (Western and CJK languages)
- Automatic font fallback system
- Glyph caching with texture atlas
- Text shaping and layout with proper kerning

### Widget Library

- **Input**: `Button`, `TextInput`, `SpinBox`, `ComboBox`
- **Display**: `TextLabel`, `TextBlock`, `Image`
- **Containers**: `Frame`, `GroupBox`, `ScrollArea`, `Widget`
- **Focus Management**: Tab navigation, keyboard control
- **Context Menus**: Right-click menu support

### Rendering System

- Command-based rendering pipeline
- Rounded rectangles with configurable corner radii
- Nine-slice image scaling
- Line, triangle, and circle primitives
- Clip rect support
- DPI scaling

### Window Management

- Multiple window types: Main, Dialog, Tool Window
- Modal dialog support
- Event system with mouse, keyboard, and scroll events
- IME (Input Method Editor) support
- Window content callback system

### Theming

- **Pro Tools Dark**: Dark theme with customizable colors
- **Pro Tools Classic**: Light theme
- Extensible style system for custom themes

## Architecture

### Core Components

```
YuchenUI/
├── core/           # Types, colors, validation, config
├── events/         # Event system and manager
├── rendering/      # Graphics context, render list
├── text/           # Font management, text rendering
├── theme/          # UI styles and theming
├── widgets/        # UI components
├── windows/        # Window management
├── platform/       # Platform-specific implementations
├── menu/           # Native menu system
└── utils/          # Clipboard and utilities
```

### Platform Layer

- **macOS**: `MacEventManager`, `MetalRenderer`, `MacWindowImpl`, `MacMenuImpl`
- **Windows**: `Win32EventManager`, `D3D11Renderer`, `Win32WindowImpl`, `Win32MenuImpl`

### Rendering Pipeline

1. Application updates widgets
2. Widgets add commands to `RenderList`
3. `GraphicsContext` executes render commands
4. Platform renderer (Metal/D3D11) draws to screen

### Text Rendering Pipeline

1. Text segmentation by script (Western/CJK)
2. HarfBuzz text shaping
3. FreeType glyph rasterization
4. Glyph caching in texture atlas
5. Vertex generation for GPU rendering

## Dependencies

### Required

- **FreeType**: Font rasterization
- **HarfBuzz**: Text shaping
- **stb_image**: PNG decoding (embedded)

### Platform-Specific

- **macOS**: Metal, AppKit, CoreText
- **Windows**: Direct3D 11, Win32 API

## Building

The framework requires:

- C++11 or later
- CMake (build system not included in provided code)
- Platform SDK (macOS SDK or Windows SDK)

## Configuration

Key configuration options in `core/Config.h`:

```cpp
namespace Config {
    namespace Window {
        static constexpr int MIN_SIZE = 1;
        static constexpr int MAX_SIZE = 8192;
    }
    
    namespace Font {
        static constexpr float MIN_SIZE = 1.0f;
        static constexpr float MAX_SIZE = 500.0f;
        static constexpr float DEFAULT_SIZE = 11.0f;
    }
}
```

## Usage Example

```cpp
#include "YuchenUI/YuchenUI.h"

class MyWindowContent : public IWindowContent {
public:
    void onCreate(Window* window, const Rect& contentArea) override {
        // Create a button
        auto* button = new Button(Rect(10, 10, 100, 30));
        button->setText("Click Me");
        button->setClickCallback([]() {
            std::cout << "Button clicked!" << std::endl;
        });
        addComponent(button);
        
        // Create a text input
        auto* input = new TextInput(Rect(10, 50, 200, 25));
        input->setPlaceholder("Enter text...");
        addComponent(input);
    }
};

int main() {
    WindowManager& wm = WindowManager::getInstance();
    wm.initialize();
    
    // Create main window with content
    wm.createMainWindow<MyWindowContent>(800, 600, "My Application");
    
    wm.run();
    return 0;
}
```

## Debug Features

When compiled with `YUCHEN_DEBUG`:

- Assertion system with detailed error messages
- Performance monitoring with frame statistics
- Validation of geometric parameters
- Resource tracking

## Font System

Built-in fonts:

- Arial Regular/Bold (Western text)
- Arial Narrow Regular/Bold
- System fonts for CJK (PingFang SC on macOS, Microsoft YaHei on Windows)

Font selection is automatic based on Unicode script detection.

## Resource System

Embedded resources are compiled into the binary:

- UI component images (buttons, scrollbars, dropdowns)
- Fonts (Arial family)
- Accessible via `Resources::findResource()`

## Image Support

- **Formats**: PNG (via stb_image)
- **Scale Modes**: Original, Stretch, Fill, Nine-Slice
- **Texture Caching**: Automatic DPI-aware caching

## Version

Version information is defined in `yuchen_version.h` (not provided in source).

## License

License information not specified in provided source code.

## Notes

- All rendering coordinates are in logical pixels (DPI-independent)
- Text input supports composition events for IME
- Scrollbars are custom-rendered, not native
- Menu system uses native platform menus
- Thread safety: Framework is designed for single-threaded use
