# ====================================================================================
# YuchenUI Testing Framework
# ====================================================================================

include(FetchContent)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

# Suppress GoogleTest warnings
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-everything")
endif()

FetchContent_MakeAvailable(googletest)

# ====================================================================================
# Test Creation Function
# ====================================================================================

function(yuchen_add_test test_name)
    add_executable(${test_name} ${ARGN})
    
    # Link test libraries
    target_link_libraries(${test_name} PRIVATE
        gtest
        gtest_main
        gmock
        gmock_main
        YuchenUI-Desktop
    )
    
    # Apply standard configuration
    yuchen_configure_target(${test_name})

    # Register test with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30
        LABELS "unit"
    )

endfunction()