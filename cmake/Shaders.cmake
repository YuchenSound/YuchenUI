if(APPLE)
    find_program(YUCHEN_XCRUN xcrun)
    if(NOT YUCHEN_XCRUN)
        message(FATAL_ERROR "xcrun not found")
    endif()
    
    function(yuchen_compile_shaders target_name)
        set(YUCHEN_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders)
        
        set(YUCHEN_BASIC_SHADER_SOURCE ${YUCHEN_SHADER_DIR}/Basic.metal)
        set(YUCHEN_BASIC_SHADER_AIR ${CMAKE_CURRENT_BINARY_DIR}/Basic.air)
        set(YUCHEN_BASIC_SHADER_METALLIB ${CMAKE_CURRENT_BINARY_DIR}/Basic.metallib)
        
        set(YUCHEN_TEXT_SHADER_SOURCE ${YUCHEN_SHADER_DIR}/Text.metal)
        set(YUCHEN_TEXT_SHADER_AIR ${CMAKE_CURRENT_BINARY_DIR}/Text.air)
        set(YUCHEN_TEXT_SHADER_METALLIB ${CMAKE_CURRENT_BINARY_DIR}/Text.metallib)
        
        set(YUCHEN_IMAGE_SHADER_SOURCE ${YUCHEN_SHADER_DIR}/Image.metal)
        set(YUCHEN_IMAGE_SHADER_AIR ${CMAKE_CURRENT_BINARY_DIR}/Image.air)
        set(YUCHEN_IMAGE_SHADER_METALLIB ${CMAKE_CURRENT_BINARY_DIR}/Image.metallib)
        
        set(YUCHEN_SHAPE_SHADER_SOURCE ${YUCHEN_SHADER_DIR}/Shape.metal)
        set(YUCHEN_SHAPE_SHADER_AIR ${CMAKE_CURRENT_BINARY_DIR}/Shape.air)
        set(YUCHEN_SHAPE_SHADER_METALLIB ${CMAKE_CURRENT_BINARY_DIR}/Shape.metallib)
        
        if(NOT EXISTS ${YUCHEN_BASIC_SHADER_SOURCE} OR NOT EXISTS ${YUCHEN_TEXT_SHADER_SOURCE} OR NOT EXISTS ${YUCHEN_IMAGE_SHADER_SOURCE} OR NOT EXISTS ${YUCHEN_SHAPE_SHADER_SOURCE})
            message(FATAL_ERROR "Shader source files not found")
        endif()
        
        add_custom_command(
            OUTPUT ${YUCHEN_BASIC_SHADER_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${YUCHEN_BASIC_SHADER_SOURCE} 
                    -o ${YUCHEN_BASIC_SHADER_AIR} -I${CMAKE_CURRENT_SOURCE_DIR}/source/include -Wall -Wextra
            DEPENDS ${YUCHEN_BASIC_SHADER_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_BASIC_SHADER_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${YUCHEN_BASIC_SHADER_AIR} -o ${YUCHEN_BASIC_SHADER_METALLIB}
            DEPENDS ${YUCHEN_BASIC_SHADER_AIR}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_TEXT_SHADER_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${YUCHEN_TEXT_SHADER_SOURCE} 
                    -o ${YUCHEN_TEXT_SHADER_AIR} -I${CMAKE_CURRENT_SOURCE_DIR}/source/include -Wall -Wextra
            DEPENDS ${YUCHEN_TEXT_SHADER_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_TEXT_SHADER_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${YUCHEN_TEXT_SHADER_AIR} -o ${YUCHEN_TEXT_SHADER_METALLIB}
            DEPENDS ${YUCHEN_TEXT_SHADER_AIR}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_IMAGE_SHADER_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${YUCHEN_IMAGE_SHADER_SOURCE} 
                    -o ${YUCHEN_IMAGE_SHADER_AIR} -I${CMAKE_CURRENT_SOURCE_DIR}/source/include -Wall -Wextra
            DEPENDS ${YUCHEN_IMAGE_SHADER_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_IMAGE_SHADER_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${YUCHEN_IMAGE_SHADER_AIR} -o ${YUCHEN_IMAGE_SHADER_METALLIB}
            DEPENDS ${YUCHEN_IMAGE_SHADER_AIR}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_SHAPE_SHADER_AIR}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metal -c ${YUCHEN_SHAPE_SHADER_SOURCE} 
                    -o ${YUCHEN_SHAPE_SHADER_AIR} -I${CMAKE_CURRENT_SOURCE_DIR}/source/include -Wall -Wextra
            DEPENDS ${YUCHEN_SHAPE_SHADER_SOURCE}
            VERBATIM
        )
        
        add_custom_command(
            OUTPUT ${YUCHEN_SHAPE_SHADER_METALLIB}
            COMMAND ${YUCHEN_XCRUN} -sdk macosx metallib ${YUCHEN_SHAPE_SHADER_AIR} -o ${YUCHEN_SHAPE_SHADER_METALLIB}
            DEPENDS ${YUCHEN_SHAPE_SHADER_AIR}
            VERBATIM
        )
        
        add_custom_target(${target_name}_shaders ALL 
            DEPENDS ${YUCHEN_BASIC_SHADER_METALLIB} ${YUCHEN_TEXT_SHADER_METALLIB} ${YUCHEN_IMAGE_SHADER_METALLIB} ${YUCHEN_SHAPE_SHADER_METALLIB})
        
        add_dependencies(${target_name} ${target_name}_shaders)
        
        set_property(GLOBAL PROPERTY YUCHEN_BASIC_SHADER_PATH ${YUCHEN_BASIC_SHADER_METALLIB})
        set_property(GLOBAL PROPERTY YUCHEN_TEXT_SHADER_PATH ${YUCHEN_TEXT_SHADER_METALLIB})
        set_property(GLOBAL PROPERTY YUCHEN_IMAGE_SHADER_PATH ${YUCHEN_IMAGE_SHADER_METALLIB})
        set_property(GLOBAL PROPERTY YUCHEN_SHAPE_SHADER_PATH ${YUCHEN_SHAPE_SHADER_METALLIB})
        
        set(YUCHEN_BASIC_SHADER_METALLIB ${YUCHEN_BASIC_SHADER_METALLIB} PARENT_SCOPE)
        set(YUCHEN_TEXT_SHADER_METALLIB ${YUCHEN_TEXT_SHADER_METALLIB} PARENT_SCOPE)
        set(YUCHEN_IMAGE_SHADER_METALLIB ${YUCHEN_IMAGE_SHADER_METALLIB} PARENT_SCOPE)
        set(YUCHEN_SHAPE_SHADER_METALLIB ${YUCHEN_SHAPE_SHADER_METALLIB} PARENT_SCOPE)
        
        yuchen_log("Configured shaders for ${target_name}")
    endfunction()
    
    function(yuchen_copy_shaders_to_bundle target_name)
        get_target_property(IS_BUNDLE ${target_name} MACOSX_BUNDLE)
        if(NOT IS_BUNDLE)
            return()
        endif()
        
        get_property(BASIC_SHADER GLOBAL PROPERTY YUCHEN_BASIC_SHADER_PATH)
        get_property(TEXT_SHADER GLOBAL PROPERTY YUCHEN_TEXT_SHADER_PATH)
        get_property(IMAGE_SHADER GLOBAL PROPERTY YUCHEN_IMAGE_SHADER_PATH)
        get_property(SHAPE_SHADER GLOBAL PROPERTY YUCHEN_SHAPE_SHADER_PATH)
        
        if(NOT BASIC_SHADER OR NOT TEXT_SHADER OR NOT IMAGE_SHADER OR NOT SHAPE_SHADER)
            message(FATAL_ERROR "Shader paths not found")
        endif()
        
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BASIC_SHADER}"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/Basic.metallib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TEXT_SHADER}"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/Text.metallib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${IMAGE_SHADER}"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/Image.metallib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHAPE_SHADER}"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>/Resources/Shape.metallib"
            VERBATIM
        )
        
        add_dependencies(${target_name} YuchenUI_shaders)
    endfunction()
    
elseif(WIN32)
    # 查找DirectX着色器编译器
    find_program(YUCHEN_FXC fxc 
        HINTS 
            "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22000.0/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/*/x64"
        DOC "DirectX Shader Compiler (fxc.exe)"
    )
    
    if(YUCHEN_FXC)
        message(STATUS "Found fxc: ${YUCHEN_FXC}")
    else()
        message(WARNING "fxc.exe not found. Shaders will not be compiled.")
    endif()
    
    function(yuchen_compile_shaders target_name)
        if(NOT YUCHEN_FXC)
            message(STATUS "Skipping shader compilation - fxc.exe not found")
            yuchen_log("Shader compilation skipped - fxc.exe not found")
            return()
        endif()
        
        set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders)
        set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
        
        file(MAKE_DIRECTORY ${OUTPUT_DIR})
        
        # Basic.hlsl - Rectangle shader
        set(BASIC_VS ${OUTPUT_DIR}/BasicVS.cso)
        set(BASIC_PS ${OUTPUT_DIR}/BasicPS.cso)
        
        if(EXISTS "${SHADER_DIR}/Basic.hlsl")
            add_custom_command(
                OUTPUT ${BASIC_VS}
                COMMAND ${YUCHEN_FXC} /T vs_5_0 /E VSMain /Fo ${BASIC_VS} ${SHADER_DIR}/Basic.hlsl
                DEPENDS ${SHADER_DIR}/Basic.hlsl
                COMMENT "Compiling Basic Vertex Shader"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${BASIC_PS}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSMain /Fo ${BASIC_PS} ${SHADER_DIR}/Basic.hlsl
                DEPENDS ${SHADER_DIR}/Basic.hlsl
                COMMENT "Compiling Basic Pixel Shader"
                VERBATIM
            )
            
            list(APPEND COMPILED_SHADERS ${BASIC_VS} ${BASIC_PS})
            set_property(GLOBAL PROPERTY YUCHEN_BASIC_VS_PATH ${BASIC_VS})
            set_property(GLOBAL PROPERTY YUCHEN_BASIC_PS_PATH ${BASIC_PS})
        endif()
        
        # Image.hlsl - Image rendering shader
        set(IMAGE_VS ${OUTPUT_DIR}/ImageVS.cso)
        set(IMAGE_PS ${OUTPUT_DIR}/ImagePS.cso)
        
        if(EXISTS "${SHADER_DIR}/Image.hlsl")
            add_custom_command(
                OUTPUT ${IMAGE_VS}
                COMMAND ${YUCHEN_FXC} /T vs_5_0 /E VSMain /Fo ${IMAGE_VS} ${SHADER_DIR}/Image.hlsl
                DEPENDS ${SHADER_DIR}/Image.hlsl
                COMMENT "Compiling Image Vertex Shader"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${IMAGE_PS}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSMain /Fo ${IMAGE_PS} ${SHADER_DIR}/Image.hlsl
                DEPENDS ${SHADER_DIR}/Image.hlsl
                COMMENT "Compiling Image Pixel Shader"
                VERBATIM
            )
            
            list(APPEND COMPILED_SHADERS ${IMAGE_VS} ${IMAGE_PS})
            set_property(GLOBAL PROPERTY YUCHEN_IMAGE_VS_PATH ${IMAGE_VS})
            set_property(GLOBAL PROPERTY YUCHEN_IMAGE_PS_PATH ${IMAGE_PS})
        endif()
        
        # Shape.hlsl - Lines and circles
        set(SHAPE_VS ${OUTPUT_DIR}/ShapeVS.cso)
        set(SHAPE_PS ${OUTPUT_DIR}/ShapePS.cso)
        set(CIRCLE_VS ${OUTPUT_DIR}/CircleVS.cso)
        set(CIRCLE_PS ${OUTPUT_DIR}/CirclePS.cso)
        
        if(EXISTS "${SHADER_DIR}/Shape.hlsl")
            add_custom_command(
                OUTPUT ${SHAPE_VS}
                COMMAND ${YUCHEN_FXC} /T vs_5_0 /E VSShape /Fo ${SHAPE_VS} ${SHADER_DIR}/Shape.hlsl
                DEPENDS ${SHADER_DIR}/Shape.hlsl
                COMMENT "Compiling Shape Vertex Shader"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${SHAPE_PS}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSShape /Fo ${SHAPE_PS} ${SHADER_DIR}/Shape.hlsl
                DEPENDS ${SHADER_DIR}/Shape.hlsl
                COMMENT "Compiling Shape Pixel Shader"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${CIRCLE_VS}
                COMMAND ${YUCHEN_FXC} /T vs_5_0 /E VSCircle /Fo ${CIRCLE_VS} ${SHADER_DIR}/Shape.hlsl
                DEPENDS ${SHADER_DIR}/Shape.hlsl
                COMMENT "Compiling Circle Vertex Shader"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${CIRCLE_PS}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSCircle /Fo ${CIRCLE_PS} ${SHADER_DIR}/Shape.hlsl
                DEPENDS ${SHADER_DIR}/Shape.hlsl
                COMMENT "Compiling Circle Pixel Shader"
                VERBATIM
            )
            
            list(APPEND COMPILED_SHADERS ${SHAPE_VS} ${SHAPE_PS} ${CIRCLE_VS} ${CIRCLE_PS})
            set_property(GLOBAL PROPERTY YUCHEN_SHAPE_VS_PATH ${SHAPE_VS})
            set_property(GLOBAL PROPERTY YUCHEN_SHAPE_PS_PATH ${SHAPE_PS})
            set_property(GLOBAL PROPERTY YUCHEN_CIRCLE_VS_PATH ${CIRCLE_VS})
            set_property(GLOBAL PROPERTY YUCHEN_CIRCLE_PS_PATH ${CIRCLE_PS})
        endif()
        
        # Text.hlsl - Text rendering (basic version)
        set(TEXT_VS ${OUTPUT_DIR}/TextVS.cso)
        set(TEXT_PS ${OUTPUT_DIR}/TextPS.cso)
        set(TEXT_PS_SUBPIXEL ${OUTPUT_DIR}/TextPS_Subpixel.cso)
        set(TEXT_PS_SDF ${OUTPUT_DIR}/TextPS_SDF.cso)
        
        if(EXISTS "${SHADER_DIR}/Text.hlsl")
            add_custom_command(
                OUTPUT ${TEXT_VS}
                COMMAND ${YUCHEN_FXC} /T vs_5_0 /E VSMain /Fo ${TEXT_VS} ${SHADER_DIR}/Text.hlsl
                DEPENDS ${SHADER_DIR}/Text.hlsl
                COMMENT "Compiling Text Vertex Shader"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${TEXT_PS}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSMain /Fo ${TEXT_PS} ${SHADER_DIR}/Text.hlsl
                DEPENDS ${SHADER_DIR}/Text.hlsl
                COMMENT "Compiling Text Pixel Shader (Basic)"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${TEXT_PS_SUBPIXEL}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSSubpixel /Fo ${TEXT_PS_SUBPIXEL} ${SHADER_DIR}/Text.hlsl
                DEPENDS ${SHADER_DIR}/Text.hlsl
                COMMENT "Compiling Text Pixel Shader (Subpixel)"
                VERBATIM
            )
            
            add_custom_command(
                OUTPUT ${TEXT_PS_SDF}
                COMMAND ${YUCHEN_FXC} /T ps_5_0 /E PSSDF /Fo ${TEXT_PS_SDF} ${SHADER_DIR}/Text.hlsl
                DEPENDS ${SHADER_DIR}/Text.hlsl
                COMMENT "Compiling Text Pixel Shader (SDF)"
                VERBATIM
            )
            
            list(APPEND COMPILED_SHADERS ${TEXT_VS} ${TEXT_PS} ${TEXT_PS_SUBPIXEL} ${TEXT_PS_SDF})
            set_property(GLOBAL PROPERTY YUCHEN_TEXT_VS_PATH ${TEXT_VS})
            set_property(GLOBAL PROPERTY YUCHEN_TEXT_PS_PATH ${TEXT_PS})
            set_property(GLOBAL PROPERTY YUCHEN_TEXT_PS_SUBPIXEL_PATH ${TEXT_PS_SUBPIXEL})
            set_property(GLOBAL PROPERTY YUCHEN_TEXT_PS_SDF_PATH ${TEXT_PS_SDF})
        endif()
        
        # 创建着色器编译目标
        if(COMPILED_SHADERS)
            add_custom_target(${target_name}_shaders ALL 
                DEPENDS ${COMPILED_SHADERS}
            )
            add_dependencies(${target_name} ${target_name}_shaders)
            
            # 复制编译好的着色器到输出目录
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${OUTPUT_DIR}
                $<TARGET_FILE_DIR:${target_name}>/shaders
                COMMENT "Copying compiled shaders to output directory"
            )
            
            yuchen_log("Configured HLSL shaders for ${target_name}")
            message(STATUS "Compiled shaders: ${COMPILED_SHADERS}")
        else()
            message(STATUS "No HLSL shader files found in ${SHADER_DIR}")
        endif()
    endfunction()
    
    function(yuchen_copy_shaders_to_bundle target_name)
        # Windows不需要bundle操作
    endfunction()
    
else()
    function(yuchen_compile_shaders target_name)
        message(WARNING "Shader compilation not supported on this platform")
    endfunction()
    
    function(yuchen_copy_shaders_to_bundle target_name)
    endfunction()
endif()