cmake_minimum_required(VERSION 3.20)
project(resource_generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(resource_generator main.cpp)

if(MSVC)
    target_compile_options(resource_generator PRIVATE
        /W4
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /wd4100
    )
    target_compile_definitions(resource_generator PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
elseif(APPLE OR UNIX)
    target_compile_options(resource_generator PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
        -pedantic
    )
endif()

install(TARGETS resource_generator
    RUNTIME DESTINATION bin
)