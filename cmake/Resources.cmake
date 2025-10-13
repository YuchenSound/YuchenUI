set(RESOURCE_GENERATOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/tools/resource_generator")
set(RESOURCE_GENERATOR_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tools/resource_generator")

if(NOT TARGET resource_generator)
    add_subdirectory(${RESOURCE_GENERATOR_DIR} ${RESOURCE_GENERATOR_BINARY_DIR})
endif()

function(yuchen_generate_resources target_name)
    set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../resources")
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    
    set(HEADER_FILE "${OUTPUT_DIR}/embedded_resources.h")
    set(SOURCE_FILE "${OUTPUT_DIR}/embedded_resources.cpp")
    
    file(GLOB_RECURSE RESOURCE_FILES "${RESOURCE_DIR}/*")
    list(FILTER RESOURCE_FILES EXCLUDE REGEX ".*\\.(DS_Store|gitkeep)$")
    list(FILTER RESOURCE_FILES EXCLUDE REGEX ".*/shaders/.*")
    
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
    
    add_custom_target(${target_name}_resources DEPENDS ${HEADER_FILE} ${SOURCE_FILE})
    
    target_sources(${target_name} PRIVATE ${SOURCE_FILE})

    add_dependencies(${target_name} ${target_name}_resources)
    
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${OUTPUT_DIR})
endfunction()