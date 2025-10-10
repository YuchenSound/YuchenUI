if(APPLE)
    find_program(YUCHEN_XCRUN xcrun REQUIRED)
    
    function(yuchen_compile_shaders target_name)
        set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../resources/shaders)
        set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
        
        set(BASIC_SOURCE ${SHADER_DIR}/Basic.metal)
        set(BASIC_AIR ${BUILD_DIR}/Basic.air)
        set(BASIC_METALLIB ${BUILD_DIR}/Basic.metallib)
        
        set(TEXT_SOURCE ${SHADER_DIR}/Text.metal)
        set(TEXT_AIR ${BUILD_DIR}/Text.air)
        set(TEXT_METALLIB ${BUILD_DIR}/Text.metallib)
        
        set(IMAGE_SOURCE ${SHADER_DIR}/Image.metal)
        set(IMAGE_AIR ${BUILD_DIR}/Image.air)
        set(IMAGE_METALLIB ${BUILD_DIR}/Image.metallib)
        
        set(SHAPE_SOURCE ${SHADER_DIR}/Shape.metal)
        set(SHAPE_AIR ${BUILD_DIR}/Shape.air)
        set(SHAPE_METALLIB ${BUILD_DIR}/Shape.metallib)
        
        add_custom_command(
            OUTPUT ${BASIC_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${BASIC_SOURCE}
                    -o ${BASIC_AIR} -Wall -Wextra
            DEPENDS ${BASIC_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${BASIC_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${BASIC_AIR}
                    -o ${BASIC_METALLIB}
            DEPENDS ${BASIC_AIR}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${TEXT_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${TEXT_SOURCE}
                    -o ${TEXT_AIR} -Wall -Wextra
            DEPENDS ${TEXT_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${TEXT_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${TEXT_AIR}
                    -o ${TEXT_METALLIB}
            DEPENDS ${TEXT_AIR}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${IMAGE_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${IMAGE_SOURCE}
                    -o ${IMAGE_AIR} -Wall -Wextra
            DEPENDS ${IMAGE_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${IMAGE_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${IMAGE_AIR}
                    -o ${IMAGE_METALLIB}
            DEPENDS ${IMAGE_AIR}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${SHAPE_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${SHAPE_SOURCE}
                    -o ${SHAPE_AIR} -Wall -Wextra
            DEPENDS ${SHAPE_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${SHAPE_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${SHAPE_AIR}
                    -o ${SHAPE_METALLIB}
            DEPENDS ${SHAPE_AIR}
            VERBATIM
        )
        
        add_custom_target(${target_name}_shaders ALL
            DEPENDS ${BASIC_METALLIB} ${TEXT_METALLIB} ${IMAGE_METALLIB} ${SHAPE_METALLIB}
        )
        
        add_dependencies(${target_name} ${target_name}_shaders)
        
        # Export shader paths for external use
        set(YUCHEN_SHADER_DIR ${BUILD_DIR} PARENT_SCOPE)
    endfunction()
    
    function(yuchen_copy_shaders_to_bundle target_name)
        get_target_property(IS_BUNDLE ${target_name} MACOSX_BUNDLE)
        if(NOT IS_BUNDLE)
            return()
        endif()
        
        # Find the YuchenUI-Desktop target's binary directory
        # This works whether YuchenUI is a subdirectory or external project
        get_target_property(DESKTOP_BINARY_DIR YuchenUI-Desktop BINARY_DIR)
        
        if(NOT DESKTOP_BINARY_DIR)
            message(WARNING "Cannot find YuchenUI-Desktop binary directory, shader copy skipped")
            return()
        endif()
        
        set(SHADER_SOURCE_DIR "${DESKTOP_BINARY_DIR}")
        
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SHADER_SOURCE_DIR}/Basic.metallib"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SHADER_SOURCE_DIR}/Text.metallib"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SHADER_SOURCE_DIR}/Image.metallib"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SHADER_SOURCE_DIR}/Shape.metallib"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/"
            COMMENT "Copying Metal shaders to app bundle"
            VERBATIM
        )
        
        add_dependencies(${target_name} YuchenUI-Desktop_shaders)
    endfunction()
    
elseif(WIN32)
    find_program(YUCHEN_FXC fxc
        HINTS "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x64"
    )
    
    function(yuchen_compile_shaders target_name)
        if(NOT YUCHEN_FXC)
            return()
        endif()
        
        set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../resources/shaders)
        set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
        
        file(MAKE_DIRECTORY ${OUTPUT_DIR})
        
        set(BASIC_VS ${OUTPUT_DIR}/BasicVS.cso)
        set(BASIC_PS ${OUTPUT_DIR}/BasicPS.cso)
        
        add_custom_command(
            OUTPUT ${BASIC_VS}
            COMMAND ${YUCHEN_FXC} /T vs_5_0 /E VSMain /Fo ${BASIC_VS}
                    ${SHADER_DIR}/Basic.hlsl
            DEPENDS ${SHADER_DIR}/Basic.hlsl
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${BASIC_PS}
            COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSMain /Fo ${BASIC_PS}
                    ${SHADER_DIR}/Basic.hlsl
            DEPENDS ${SHADER_DIR}/Basic.hlsl
            VERBATIM
        )
        
        add_custom_target(${target_name}_shaders ALL
            DEPENDS ${BASIC_VS} ${BASIC_PS}
        )
        
        add_dependencies(${target_name} ${target_name}_shaders)
        
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${OUTPUT_DIR}
                    $<TARGET_FILE_DIR:${target_name}>/shaders
        )
    endfunction()
    
    function(yuchen_copy_shaders_to_bundle target_name)
    endfunction()
endif()