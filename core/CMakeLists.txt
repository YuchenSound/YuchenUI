project(YuchenUI)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(Compiler)
include(Text)
include(Image)
include(Resources)

file(GLOB_RECURSE YUCHEN_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE YUCHEN_CORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/YuchenUI/*.h"
)

add_library(YuchenUI STATIC 
    ${YUCHEN_CORE_SOURCES} 
    ${YUCHEN_CORE_HEADERS}
)

yuchen_configure_target(YuchenUI)
yuchen_configure_text_libraries(YuchenUI)
yuchen_configure_image_libraries(YuchenUI)
yuchen_generate_resources(YuchenUI)

target_include_directories(YuchenUI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(YuchenUI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
             PREFIX "Header Files"
             FILES ${YUCHEN_CORE_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
             PREFIX "Source Files"
             FILES ${YUCHEN_CORE_SOURCES})