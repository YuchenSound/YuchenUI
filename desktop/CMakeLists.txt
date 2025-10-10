project(YuchenUI-Desktop)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(Compiler)
include(Dependencies)
include(Shaders)

# Scan cross-platform sources
file(GLOB_RECURSE YUCHEN_DESKTOP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/menu/*.cpp"
)

# Exclude platform-specific subdirectories
list(FILTER YUCHEN_DESKTOP_SOURCES EXCLUDE REGEX ".*/platform/.*")
list(FILTER YUCHEN_DESKTOP_SOURCES EXCLUDE REGEX ".*/utils/.*")

# Add platform-specific sources
if(APPLE)
    file(GLOB_RECURSE YUCHEN_PLATFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos/*.mm"
    )
    
    list(APPEND YUCHEN_PLATFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Clipboard_macOS.mm"
    )
    
    set_source_files_properties(${YUCHEN_PLATFORM_SOURCES}
        PROPERTIES COMPILE_FLAGS "-fobjc-arc -fobjc-exceptions")
        
elseif(WIN32)
    file(GLOB_RECURSE YUCHEN_PLATFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/*.cpp"
    )
    
    list(APPEND YUCHEN_PLATFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Clipboard_Windows.cpp"
    )
endif()

# Scan public headers only (in include directory)
file(GLOB_RECURSE YUCHEN_DESKTOP_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/YuchenUI/*.h"
)

# Create library target
add_library(YuchenUI-Desktop STATIC
    ${YUCHEN_DESKTOP_SOURCES}
    ${YUCHEN_PLATFORM_SOURCES}
    ${YUCHEN_DESKTOP_HEADERS}
)

yuchen_configure_target(YuchenUI-Desktop)
yuchen_link_dependencies(YuchenUI-Desktop)
yuchen_compile_shaders(YuchenUI-Desktop)

target_link_libraries(YuchenUI-Desktop PUBLIC YuchenUI)

# Public headers
target_include_directories(YuchenUI-Desktop PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Private headers (for WindowManagerPlatform.h)
target_include_directories(YuchenUI-Desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/windows
)

# Source groups for IDE
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
             PREFIX "Header Files"
             FILES ${YUCHEN_DESKTOP_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
             PREFIX "Source Files"
             FILES ${YUCHEN_DESKTOP_SOURCES} ${YUCHEN_PLATFORM_SOURCES})