#pragma once

//==========================================================================================
// YuchenUI Desktop Platform
//
// Complete desktop windowing integration for YuchenUI.
// Includes all Core UI components plus platform-specific window management.
//==========================================================================================

// Core UI components
#include "YuchenUI/YuchenUI.h"

// Desktop window management
#include "YuchenUI/windows/Window.h"
#include "YuchenUI/windows/BaseWindow.h"
#include "YuchenUI/windows/WindowManager.h"

/**
 * Usage:
 *
 * #include <YuchenUI/YuchenUI-Desktop.h>
 *
 * YuchenUI::BaseWindow window;
 * window.create(800, 600, "My App");
 *
 * auto content = std::make_unique<MyContent>();
 * window.setContent(std::move(content));
 *
 * YuchenUI::WindowManager::getInstance().run();
 */
