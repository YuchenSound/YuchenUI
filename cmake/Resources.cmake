find_program(PYTHON_EXECUTABLE python3 python REQUIRED)

function(yuchen_generate_resources target_name)
    set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    set(SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/generate_resources.py")
    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    
    set(HEADER_FILE "${OUTPUT_DIR}/embedded_resources.h")
    set(SOURCE_FILE "${OUTPUT_DIR}/embedded_resources.cpp")
    set(INDEX_FILE "${OUTPUT_DIR}/resource_index.json")
    
    file(GLOB_RECURSE RESOURCE_FILES "${RESOURCE_DIR}/*")
    list(FILTER RESOURCE_FILES EXCLUDE REGEX ".*\\.(DS_Store|gitkeep)$")
    list(FILTER RESOURCE_FILES EXCLUDE REGEX ".*/shaders/.*")
    
    add_custom_command(
        OUTPUT ${HEADER_FILE} ${SOURCE_FILE} ${INDEX_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${PYTHON_EXECUTABLE} ${SCRIPT_PATH}
            --input-dir "${RESOURCE_DIR}"
            --output-dir "${OUTPUT_DIR}"
            --namespace "YuchenUI::Resources"
            --header-file "embedded_resources.h"
            --source-file "embedded_resources.cpp"
        DEPENDS ${SCRIPT_PATH} ${RESOURCE_FILES}
        COMMENT "Generating embedded resources"
        VERBATIM
    )
    
    add_custom_target(${target_name}_resources DEPENDS ${HEADER_FILE} ${SOURCE_FILE} ${INDEX_FILE})
    
    target_sources(${target_name} PRIVATE ${SOURCE_FILE})
    target_include_directories(${target_name} PRIVATE ${OUTPUT_DIR})
    add_dependencies(${target_name} ${target_name}_resources)
    
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${OUTPUT_DIR})
endfunction()