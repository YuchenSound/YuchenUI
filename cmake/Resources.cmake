set(RESOURCE_GENERATOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/tools/resource_generator")
set(RESOURCE_GENERATOR_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tools/resource_generator")

if(NOT TARGET resource_generator)
    add_subdirectory(${RESOURCE_GENERATOR_DIR} ${RESOURCE_GENERATOR_BINARY_DIR})
endif()

function(yuchen_generate_resources target_name)
    set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(HEADER_FILE "${OUTPUT_DIR}/embedded_resources.h")
    set(SOURCE_FILE "${OUTPUT_DIR}/embedded_resources.cpp")
    
    if(NOT EXISTS ${RESOURCE_DIR})
        message(WARNING "Resource directory not found: ${RESOURCE_DIR}")
        return()
    endif()
    
    file(GLOB_RECURSE RESOURCE_FILES "${RESOURCE_DIR}/*")
    list(FILTER RESOURCE_FILES EXCLUDE REGEX ".*\\.(DS_Store|gitkeep)$")
    
    if(NOT RESOURCE_FILES)
        message(WARNING "No resources found in: ${RESOURCE_DIR}")
        return()
    endif()
    
    add_custom_command(
        OUTPUT ${HEADER_FILE} ${SOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND resource_generator
            --input-dir "${RESOURCE_DIR}"
            --output-dir "${OUTPUT_DIR}"
            --namespace "YuchenUI::Resources"
            --header-file "embedded_resources.h"
            --source-file "embedded_resources.cpp"
        DEPENDS resource_generator ${RESOURCE_FILES}
        COMMENT "Generating embedded resources for ${target_name}"
        VERBATIM
    )
    
    target_sources(${target_name} PRIVATE ${SOURCE_FILE})
    target_include_directories(${target_name} PUBLIC
        $<BUILD_INTERFACE:${OUTPUT_DIR}>
    )
endfunction()

function(yuchen_add_app_resources target_name)
    set(options AUTO_REGISTER)
    set(oneValueArgs RESOURCE_DIR OUTPUT_PREFIX NAMESPACE)
    set(multiValueArgs)
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT ARG_RESOURCE_DIR)
        message(FATAL_ERROR "RESOURCE_DIR is required")
    endif()
    
    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "${target_name}::Resources")
    endif()
    
    if(NOT ARG_OUTPUT_PREFIX)
        set(ARG_OUTPUT_PREFIX "${target_name}_res")
    endif()
    
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(HEADER_FILE "${OUTPUT_DIR}/${ARG_OUTPUT_PREFIX}_resources.h")
    set(SOURCE_FILE "${OUTPUT_DIR}/${ARG_OUTPUT_PREFIX}_resources.cpp")
    
    file(GLOB_RECURSE RESOURCE_FILES "${ARG_RESOURCE_DIR}/*")
    list(FILTER RESOURCE_FILES EXCLUDE REGEX ".*\\.(DS_Store|gitkeep)$")
    
    add_custom_command(
        OUTPUT ${HEADER_FILE} ${SOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND resource_generator
            --input-dir "${ARG_RESOURCE_DIR}"
            --output-dir "${OUTPUT_DIR}"
            --namespace "${ARG_NAMESPACE}"
            --header-file "${ARG_OUTPUT_PREFIX}_resources.h"
            --source-file "${ARG_OUTPUT_PREFIX}_resources.cpp"
        DEPENDS resource_generator ${RESOURCE_FILES}
        VERBATIM
    )
    
    if(ARG_AUTO_REGISTER)
        string(REGEX REPLACE "::.*" "" NAMESPACE_NAME "${ARG_NAMESPACE}")
        
        set(REGISTER_FILE "${OUTPUT_DIR}/${ARG_OUTPUT_PREFIX}_autoregister.cpp")
        
file(WRITE ${REGISTER_FILE}
"#include \"YuchenUI/resource/ResourceManager.h\"
#include \"YuchenUI/resource/EmbeddedResourceProvider.h\"
#include \"${ARG_OUTPUT_PREFIX}_resources.h\"

namespace {
static struct AutoRegister {
    AutoRegister() {
        YuchenUI::ResourceManager::getInstance().registerProvider(
            \"${NAMESPACE_NAME}\",
            new YuchenUI::EmbeddedResourceProvider(
                reinterpret_cast<const YuchenUI::Resources::ResourceData*>(${ARG_NAMESPACE}::getAllResources()),
                ${ARG_NAMESPACE}::getResourceCount()
            )
        );
    }
} s_autoRegister;
}
")
        target_sources(${target_name} PRIVATE ${REGISTER_FILE})
    endif()
    
    target_sources(${target_name} PRIVATE ${SOURCE_FILE})
    target_include_directories(${target_name} PRIVATE ${OUTPUT_DIR})
endfunction()