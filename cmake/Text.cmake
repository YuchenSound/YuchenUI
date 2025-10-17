function(yuchen_configure_text_libraries target_name)
    set(FREETYPE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freetype")
    set(HARFBUZZ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/harfbuzz")

    set(FT_DISABLE_ZLIB ON CACHE BOOL "")
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "")
    set(FT_DISABLE_PNG ON CACHE BOOL "")
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "")
    set(FT_WITH_HARFBUZZ OFF CACHE BOOL "")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
    
    set(ORIGINAL_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    
    if(APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    elseif(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
    endif()
    
    add_subdirectory(${FREETYPE_DIR} freetype EXCLUDE_FROM_ALL)
    
    set(HB_HAVE_FREETYPE ON CACHE BOOL "")
    set(HB_BUILD_TESTS OFF CACHE BOOL "")
    set(HB_BUILD_SUBSET OFF CACHE BOOL "")
    set(HB_HAVE_GOBJECT OFF CACHE BOOL "")
    set(HB_HAVE_CAIRO OFF CACHE BOOL "")
    set(HB_HAVE_ICU OFF CACHE BOOL "")
    set(HB_BUILD_UTILS OFF CACHE BOOL "")
    set(FREETYPE_INCLUDE_DIRS "${FREETYPE_DIR}/include" CACHE PATH "")
    set(FREETYPE_LIBRARIES freetype CACHE STRING "")
    
    add_subdirectory(${HARFBUZZ_DIR} harfbuzz EXCLUDE_FROM_ALL)
    
    set(CMAKE_CXX_FLAGS ${ORIGINAL_CXX_FLAGS})
    
    target_link_libraries(${target_name} PRIVATE freetype harfbuzz)
    target_include_directories(${target_name} PUBLIC
        ${FREETYPE_DIR}/include
        ${HARFBUZZ_DIR}/src
    )
    target_compile_definitions(${target_name} PRIVATE
        YUCHEN_TEXT_FREETYPE=1
        YUCHEN_TEXT_HARFBUZZ=1
        FT2_BUILD_LIBRARY=1
        HB_HAVE_FREETYPE=1
    )
endfunction()