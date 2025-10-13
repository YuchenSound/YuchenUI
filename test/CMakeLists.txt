include(Testing)

file(GLOB_RECURSE UNIT_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
)

if(UNIT_TEST_SOURCES)
    yuchen_add_test(YuchenUI_UnitTests
        ${UNIT_TEST_SOURCES}
    )
    
    target_include_directories(YuchenUI_UnitTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()