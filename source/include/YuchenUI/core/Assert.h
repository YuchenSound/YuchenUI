#pragma once

#include <iostream>
#include <cstdlib>

namespace YuchenUI {

[[noreturn]] inline void AssertFail(const char* expr, const char* file, int line, const char* func, const char* msg = nullptr) {
    std::cerr << "\n=== YUCHEN UI ASSERTION FAILURE ===" << std::endl;
    std::cerr << "Expression: " << expr << std::endl;
    std::cerr << "File: " << file << std::endl;
    std::cerr << "Line: " << line << std::endl;
    std::cerr << "Function: " << func << std::endl;
    if (msg) {
        std::cerr << "Message: " << msg << std::endl;
    }
    std::cerr << "===================================" << std::endl;
    std::cerr.flush();
    std::abort();
}

}

#ifdef YUCHEN_DEBUG
    #define YUCHEN_ASSERT(expr) \
        do { \
            if (!(expr)) { \
                ::YuchenUI::AssertFail(#expr, __FILE__, __LINE__, __func__); \
            } \
        } while(0)

    #define YUCHEN_ASSERT_MSG(expr, msg) \
        do { \
            if (!(expr)) { \
                ::YuchenUI::AssertFail(#expr, __FILE__, __LINE__, __func__, msg); \
            } \
        } while(0)

    #define YUCHEN_UNREACHABLE() \
        ::YuchenUI::AssertFail("UNREACHABLE", __FILE__, __LINE__, __func__, \
                              "This code path should never be reached")
#else
    #define YUCHEN_ASSERT(expr) ((void)(expr))
    #define YUCHEN_ASSERT_MSG(expr, msg) ((void)(expr))
    #define YUCHEN_UNREACHABLE() ((void)0)
#endif
