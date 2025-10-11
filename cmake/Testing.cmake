include(FetchContent)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-everything")
endif()

FetchContent_MakeAvailable(googletest)

function(yuchen_add_test test_name)
    add_executable(${test_name} ${ARGN})
    
    target_link_libraries(${test_name} PRIVATE
        gtest
        gtest_main
        YuchenUI-Desktop
    )
    
    yuchen_configure_target(${test_name})
    
    # 复制着色器到测试二进制目录
    if(APPLE)
        yuchen_copy_shaders_to_bundle(${test_name})
        
        # 如果不是 bundle，需要手动复制
        get_target_property(IS_BUNDLE ${test_name} MACOSX_BUNDLE)
        if(NOT IS_BUNDLE)
            get_target_property(DESKTOP_BINARY_DIR YuchenUI-Desktop BINARY_DIR)
            add_custom_command(TARGET ${test_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DESKTOP_BINARY_DIR}/Basic.metallib"
                    "$<TARGET_FILE_DIR:${test_name}>/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DESKTOP_BINARY_DIR}/Text.metallib"
                    "$<TARGET_FILE_DIR:${test_name}>/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DESKTOP_BINARY_DIR}/Image.metallib"
                    "$<TARGET_FILE_DIR:${test_name}>/"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DESKTOP_BINARY_DIR}/Shape.metallib"
                    "$<TARGET_FILE_DIR:${test_name}>/"
                COMMENT "Copying shaders for test ${test_name}"
            )
        endif()
    endif()
    
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30
        LABELS "unit"
    )
    
    if(YUCHEN_VERBOSE_OUTPUT)
        message(STATUS "[YuchenUI] Added test: ${test_name}")
    endif()
endfunction()