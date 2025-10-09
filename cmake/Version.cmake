find_package(Git QUIET)

function(yuchen_configure_version target_name)
    set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(VERSION_BUILD "0")
    set(VERSION_COMMIT "unknown")
    set(VERSION_BRANCH "unknown")
    set(VERSION_DIRTY "")
    
    if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --long --dirty --always
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_DESCRIBE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BUILD_COUNT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        
        execute_process(
            COMMAND ${GIT_EXECUTABLE} diff --quiet
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_DIRTY_CHECK
            ERROR_QUIET
        )
        
        if(GIT_BUILD_COUNT)
            set(VERSION_BUILD ${GIT_BUILD_COUNT})
        endif()
        
        if(GIT_COMMIT)
            set(VERSION_COMMIT ${GIT_COMMIT})
        endif()
        
        if(GIT_BRANCH)
            set(VERSION_BRANCH ${GIT_BRANCH})
        endif()
        
        if(NOT GIT_DIRTY_CHECK EQUAL 0)
            set(VERSION_DIRTY "-dirty")
        endif()
    endif()
    
    set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(VERSION_FULL "${VERSION_STRING}+${VERSION_BUILD}.${VERSION_COMMIT}${VERSION_DIRTY}")
    
    set(VERSION_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(VERSION_HEADER_FILE "${VERSION_HEADER_DIR}/yuchen_version.h")
    
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/yuchen_version.h.in"
        "${VERSION_HEADER_FILE}"
        @ONLY
    )
    
    target_include_directories(${target_name} PUBLIC 
        $<BUILD_INTERFACE:${VERSION_HEADER_DIR}>
    )
    
    set_target_properties(${target_name} PROPERTIES
        VERSION ${VERSION_STRING}
        SOVERSION ${VERSION_MAJOR}
    )
    
    if(YUCHEN_VERBOSE_OUTPUT)
        message(STATUS "[YuchenUI] Version: ${VERSION_FULL}")
    endif()
endfunction()