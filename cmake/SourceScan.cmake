function(yuchen_scan_sources RESULT_VAR)
    file(GLOB_RECURSE ALL_CPP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/src/*.cpp"
    )
    
    list(FILTER ALL_CPP_SOURCES EXCLUDE REGEX ".*/platform/.*")
    
    if(APPLE)
        file(GLOB_RECURSE PLATFORM_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/src/platform/macos/*.mm"
        )
    elseif(WIN32)
        file(GLOB_RECURSE PLATFORM_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/source/src/platform/windows/*.cpp"
        )
    endif()
    
    set(${RESULT_VAR} ${ALL_CPP_SOURCES} ${PLATFORM_SOURCES} PARENT_SCOPE)
endfunction()

function(yuchen_scan_headers RESULT_VAR)
    file(GLOB_RECURSE ALL_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/include/YuchenUI/*.h"
    )
    set(${RESULT_VAR} ${ALL_HEADERS} PARENT_SCOPE)
endfunction()

function(yuchen_scan_private_headers RESULT_VAR)
    file(GLOB_RECURSE PRIVATE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/source/src/*.h"
    )
    set(${RESULT_VAR} ${PRIVATE_HEADERS} PARENT_SCOPE)
endfunction()

function(yuchen_configure_source_files TARGET_NAME SOURCE_LIST)
    if(NOT APPLE)
        return()
    endif()
    
    set(OBJCXX_FILES "")
    foreach(SOURCE_FILE ${SOURCE_LIST})
        get_filename_component(FILE_EXT ${SOURCE_FILE} EXT)
        get_filename_component(FILE_NAME ${SOURCE_FILE} NAME)
        if(FILE_EXT STREQUAL ".mm" OR FILE_NAME STREQUAL "Application.cpp")
            list(APPEND OBJCXX_FILES ${SOURCE_FILE})
        endif()
    endforeach()
    
    if(OBJCXX_FILES)
        set_source_files_properties(${OBJCXX_FILES} PROPERTIES 
            COMPILE_FLAGS "-fobjc-arc -fobjc-exceptions")
    endif()
endfunction()