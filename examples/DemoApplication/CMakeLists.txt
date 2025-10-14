# YuchenUI Demo Application
cmake_minimum_required(VERSION 3.20)
project(YuchenUIDemo VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准配置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型配置
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# UTF-8 支持 (MSVC)
if(MSVC)
    add_compile_options(/utf-8 /wd4819)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    message(STATUS "MSVC: UTF-8 编码支持已启用")
endif()

# 平台特定源文件
if(APPLE)
    set(DEMO_SOURCES
        src/Application.cpp
        src/MixerPanel/MixerWindow.cpp
        src/MixerPanel/MixerWindow.h
        src/main.cpp
    )
    
    # 添加 Objective-C++ 源文件（如果存在）
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
    message(FATAL_ERROR "不支持的平台！YuchenUI 仅支持 macOS 和 Windows")
endif()

# 创建可执行文件
if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${DEMO_SOURCES})
    
    # Objective-C++ 编译选项
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/Application.mm")
        set_source_files_properties(src/Application.mm
            PROPERTIES COMPILE_FLAGS "-fobjc-arc -fobjc-exceptions"
        )
    endif()
    
    # macOS Bundle 属性
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "YuchenUI Demo"
        MACOSX_BUNDLE_IDENTIFIER "com.yuchen.yuchenuidemo"
        MACOSX_BUNDLE_EXECUTABLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
    )
    
    # 复制着色器到 Bundle
    yuchen_copy_shaders_to_bundle(${PROJECT_NAME})
    
elseif(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${DEMO_SOURCES})
    
    if(MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE TRUE
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        )
        
        # 设置为启动项目
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
                     PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    endif()
endif()

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE 
    YuchenUI 
    YuchenUI-Desktop
)

# 编译选项 - macOS/Clang
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

# 编译选项 - MSVC
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4                 # 警告级别4
        /permissive-        # 严格标准遵从
        /utf-8              # UTF-8 编码
        /Zc:__cplusplus     # 正确的 __cplusplus 宏
        /wd4100             # 忽略未使用的参数
        /wd4819             # 忽略代码页警告
        $<$<CONFIG:Debug>:/Od /Zi>      # Debug: 无优化 + 调试信息
        $<$<CONFIG:Release>:/O2>        # Release: 优化
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()

# Windows 着色器复制
if(WIN32 AND MSVC)
    add_dependencies(${PROJECT_NAME} YuchenUI_shaders)
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "复制着色器到 $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/YuchenUI/shaders"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        COMMENT "复制编译后的着色器文件"
        VERBATIM
    )
    
    message(STATUS "着色器复制已配置: ${PROJECT_NAME}")
endif()

# 预处理器定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:YUCHEN_DEBUG=1;DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG>
)