function(yuchen_configure_image_libraries target_name)
    set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/stb")
    target_include_directories(${target_name} PRIVATE ${STB_DIR})
endfunction()