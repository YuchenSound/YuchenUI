cmake_minimum_required(VERSION 3.20)
project(YuchenUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Platform)

option(YUCHEN_BUILD_DESKTOP "Build desktop platform support" ON)

add_subdirectory(core)

if(YUCHEN_BUILD_DESKTOP)
    add_subdirectory(desktop)
endif()
