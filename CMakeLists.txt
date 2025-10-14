cmake_minimum_required(VERSION 3.20)
project(YuchenUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Platform)

option(YUCHEN_BUILD_EXAMPLES "构建示例程序" ON)
option(YUCHEN_BUILD_TESTS "构建单元测试" ON)
option(YUCHEN_WARNINGS_AS_ERRORS "将警告视为错误" OFF)

add_subdirectory(core)
add_subdirectory(desktop)

# ============================================================================
# Example Application
if(YUCHEN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# Unit Test
if(YUCHEN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()