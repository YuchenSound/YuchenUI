/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Core module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

#include <iostream>
#include <cstdlib>

namespace YuchenUI {

//==========================================================================================
/** Prints assertion failure diagnostic information.
    
    This function only prints information and returns normally.
    The actual breakpoint is triggered in the macro to ensure correct debug location.
    
    @param expr  String representation of the failed expression
    @param file  Source file path where assertion failed
    @param line  Line number where assertion failed
    @param func  Function name where assertion failed
    @param msg   Optional custom error message
*/
inline void PrintAssertFailure(const char* expr, const char* file, int line, const char* func, const char* msg = nullptr) {
    std::cerr << "\n=== YUCHEN UI ASSERTION FAILURE ===" << std::endl;
    std::cerr << "Expression: " << expr << std::endl;
    std::cerr << "File: " << file << std::endl;
    std::cerr << "Line: " << line << std::endl;
    std::cerr << "Function: " << func << std::endl;
    if (msg) std::cerr << "Message: " << msg << std::endl;
    std::cerr << "===================================" << std::endl;
    std::cerr.flush();
}

} // namespace YuchenUI

//==========================================================================================
// Platform-specific debug break macros

#if defined(_MSC_VER)
    // Windows: Use __debugbreak() intrinsic
    #define YUCHEN_DEBUG_BREAK() __debugbreak()
    
#elif defined(__clang__) || defined(__GNUC__)
    // Clang/GCC: Use __builtin_trap() which generates optimal platform code
    #define YUCHEN_DEBUG_BREAK() __builtin_trap()
    
#else
    // Fallback: Use abort() on unknown platforms
    #define YUCHEN_DEBUG_BREAK() std::abort()
#endif

//==========================================================================================
// Assertion macros - only active in debug builds (when YUCHEN_DEBUG is defined)

#ifdef YUCHEN_DEBUG
    /// Assert that expression is true, trigger breakpoint if false
    #define YUCHEN_ASSERT(expr) \
        do { \
            if (!(expr)) { \
                ::YuchenUI::PrintAssertFailure(#expr, __FILE__, __LINE__, __func__); \
                YUCHEN_DEBUG_BREAK(); \
            } \
        } while(0)

    /// Assert with custom error message
    #define YUCHEN_ASSERT_MSG(expr, msg) \
        do { \
            if (!(expr)) { \
                ::YuchenUI::PrintAssertFailure(#expr, __FILE__, __LINE__, __func__, msg); \
                YUCHEN_DEBUG_BREAK(); \
            } \
        } while(0)

    /// Mark unreachable code paths
    #define YUCHEN_UNREACHABLE() \
        do { \
            ::YuchenUI::PrintAssertFailure("UNREACHABLE", __FILE__, __LINE__, __func__, \
                                          "This code path should never be reached"); \
            YUCHEN_DEBUG_BREAK(); \
        } \
        while(0)
        
#else
    /// In release builds, assertions are compiled out
    #define YUCHEN_ASSERT(expr) ((void)(expr))
    #define YUCHEN_ASSERT_MSG(expr, msg) ((void)(expr))
    #define YUCHEN_UNREACHABLE() ((void)0)
#endif
