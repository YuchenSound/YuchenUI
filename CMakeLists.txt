cmake_minimum_required(VERSION 3.20)
project(YuchenUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Platform)
include(Dependencies)
include(SourceScan)
include(Shaders)
include(Compiler)
include(Text)
include(Image) 
include(Resources)
include(Version)

yuchen_scan_sources(YUCHEN_SOURCES)
yuchen_scan_headers(YUCHEN_HEADERS)
yuchen_scan_private_headers(YUCHEN_PRIVATE_HEADERS)

set(YUCHEN_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/include)

add_library(${PROJECT_NAME} STATIC ${YUCHEN_SOURCES} ${YUCHEN_HEADERS})

yuchen_configure_source_files(${PROJECT_NAME} "${YUCHEN_SOURCES}")
yuchen_configure_target(${PROJECT_NAME})
yuchen_configure_version(${PROJECT_NAME})
yuchen_link_dependencies(${PROJECT_NAME})
yuchen_configure_text_libraries(${PROJECT_NAME})
yuchen_configure_image_libraries(${PROJECT_NAME}) 
yuchen_compile_shaders(${PROJECT_NAME})
yuchen_generate_resources(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${YUCHEN_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/include" 
             PREFIX "Header Files" 
             FILES ${YUCHEN_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/source/src" 
             PREFIX "Source Files" 
             FILES ${YUCHEN_SOURCES})

include(Installation)