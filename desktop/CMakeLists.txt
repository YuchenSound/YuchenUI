project(YuchenUI-Desktop)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(Compiler)
include(Dependencies)
include(Resources)

file(GLOB_RECURSE YUCHEN_DESKTOP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/menu/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/YuchenUI-Desktop/*.cpp"
)

list(FILTER YUCHEN_DESKTOP_SOURCES EXCLUDE REGEX ".*/platform/.*")
list(FILTER YUCHEN_DESKTOP_SOURCES EXCLUDE REGEX ".*/utils/.*")

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

file(GLOB_RECURSE YUCHEN_DESKTOP_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/YuchenUI/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/YuchenUI-Desktop/*.h"
)

add_library(YuchenUI-Desktop STATIC
    ${YUCHEN_DESKTOP_SOURCES}
    ${YUCHEN_PLATFORM_SOURCES}
    ${YUCHEN_DESKTOP_HEADERS}
)

yuchen_configure_target(YuchenUI-Desktop)
yuchen_link_dependencies(YuchenUI-Desktop)
yuchen_generate_resources(YuchenUI-Desktop)

target_link_libraries(YuchenUI-Desktop PUBLIC YuchenUI)

target_include_directories(YuchenUI-Desktop PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(YuchenUI-Desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/windows
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
             PREFIX "Header Files"
             FILES ${YUCHEN_DESKTOP_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
             PREFIX "Source Files"
             FILES ${YUCHEN_DESKTOP_SOURCES} ${YUCHEN_PLATFORM_SOURCES})