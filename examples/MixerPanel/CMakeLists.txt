cmake_minimum_required(VERSION 3.20)
project(MixerPanel VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB_RECURSE MIXER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

if(APPLE)
    file(GLOB_RECURSE OBJCPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm")
    if(OBJCPP_SOURCES)
        list(APPEND MIXER_SOURCES ${OBJCPP_SOURCES})
    endif()
elseif(NOT WIN32)
    message(FATAL_ERROR "Unsupported platform! MixerPanel only supports macOS and Windows")
endif()

if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${MIXER_SOURCES})
    
    if(OBJCPP_SOURCES)
        set_source_files_properties(${OBJCPP_SOURCES}
            PROPERTIES COMPILE_FLAGS "-fobjc-arc -fobjc-exceptions"
        )
    endif()
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "Mixer Panel"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yuchen.mixerpanel"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_ICON_FILE ""
        MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 Yuchen Wei. All rights reserved."
    )
elseif(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${MIXER_SOURCES})
    
    if(MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        )
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
                     PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    YuchenUI 
    YuchenUI-Desktop
)

yuchen_add_app_resources(${PROJECT_NAME}
    RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    NAMESPACE "MixerPanel::Resources"
    OUTPUT_PREFIX "mixer"
    AUTO_REGISTER ON
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
        /W4
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /wd4100
        /wd4819
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:Release>:/O2>
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    YUCHEN_ENABLE_AUTO_REGISTRATION
    $<$<CONFIG:Debug>:YUCHEN_DEBUG=1;DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG>
)