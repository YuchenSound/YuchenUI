cmake_minimum_required(VERSION 3.20)
project(YuchenUIDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(MSVC)
    add_compile_options(/utf-8 /wd4819)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    message(STATUS "[YuchenUI] MSVC UTF-8 encoding enabled")
endif()


if(APPLE)
    set(DEMO_SOURCES
        src/Application.cpp
        src/MixerPanel/MixerWindow.cpp
        src/MixerPanel/MixerWindow.h
        src/main.cpp
    )
    
    # Add Objective-C++ source file if exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/Application.mm")
        list(APPEND DEMO_SOURCES src/Application.mm)
    endif()
    
elseif(WIN32)
    set(DEMO_SOURCES
        src/Application.cpp
        src/MixerPanel/MixerWindow.cpp
        src/MixerPanel/MixerWindow.h
        src/main.cpp
    )
    
else()
    message(FATAL_ERROR "Unsupported platform! YuchenUI only supports macOS and Windows")
endif()

if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${DEMO_SOURCES})
    
    # Objective-C++ compilation flags
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/Application.mm")
        set_source_files_properties(src/Application.mm
            PROPERTIES COMPILE_FLAGS "-fobjc-arc -fobjc-exceptions"
        )
    endif()
    
    # macOS Bundle properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "YuchenUI Demo"
        MACOSX_BUNDLE_IDENTIFIER "com.yuchen.yuchenuidemo"
        MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
    )
elseif(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${DEMO_SOURCES})
    
    if(MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        )
        
        # Set as startup project
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
                     PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    YuchenUI 
    YuchenUI-Desktop
)

if(APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra
        -Wno-unused-parameter 
        -Wno-missing-field-initializers 
        -pedantic
        $<$<CONFIG:Debug>:-g3 -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4                 # Warning level 4
        /permissive-        # Strict standard conformance
        /utf-8              # UTF-8 encoding
        /Zc:__cplusplus     # Correct __cplusplus macro
        /wd4100             # Ignore unused parameter
        /wd4819             # Ignore code page warning
        $<$<CONFIG:Debug>:/Od /Zi>      # Debug: no optimization + debug info
        $<$<CONFIG:Release>:/O2>        # Release: optimization
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()


target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:YUCHEN_DEBUG=1;DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG>
)

