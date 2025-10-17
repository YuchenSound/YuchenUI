include(Testing)

# 收集跨平台测试文件（.cpp）
file(GLOB_RECURSE UNIT_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
)

# 在 macOS 上添加平台特定的测试文件（.mm）
if(APPLE)
    file(GLOB MACOS_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.mm"
    )
    list(APPEND UNIT_TEST_SOURCES ${MACOS_TEST_SOURCES})
endif()

if(UNIT_TEST_SOURCES)
    yuchen_add_test(YuchenUI_UnitTests
        ${UNIT_TEST_SOURCES}
    )
    
    # 基础 include 路径
    target_include_directories(YuchenUI_UnitTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # macOS 特定配置
    if(APPLE)
        # 添加 Metal 相关的私有头文件路径
        target_include_directories(YuchenUI_UnitTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../desktop/src/platform/macos
        )
        
        # 链接 Metal 和 Cocoa 框架
        target_link_libraries(YuchenUI_UnitTests PRIVATE
            "-framework Metal"
            "-framework QuartzCore"
            "-framework Cocoa"
            "-framework AppKit"
        )
        
        # 为所有 .mm 文件设置 Objective-C++ 编译标志
        if(MACOS_TEST_SOURCES)
            set_source_files_properties(
                ${MACOS_TEST_SOURCES}
                PROPERTIES
                COMPILE_FLAGS "-fobjc-arc -fobjc-exceptions"
            )
        endif()
    endif()
endif()