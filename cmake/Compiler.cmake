function(yuchen_configure_target target_name)
    if(APPLE)
        set_target_properties(${target_name} PROPERTIES
            COMPILE_FLAGS "-x objective-c++ -fobjc-arc -fobjc-exceptions"
        )
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers -pedantic
            $<$<CONFIG:Debug>:-g3 -O0>
            $<$<CONFIG:Release>:-O3>
        )
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4              # Warning level 4
            /permissive-     # Strict conformance
            /utf-8           # UTF-8 source and execution charset
            /Zc:__cplusplus  # Correct __cplusplus macro
            /wd4100          # Unreferenced formal parameter
            /wd4819          # Code page warning (for third-party libraries)
            /wd4996          # Deprecated functions (like strncpy)
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2>
        )
        
        target_compile_definitions(${target_name} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            UNICODE
            _UNICODE
        )
        
        yuchen_log("MSVC: UTF-8 support enabled for ${target_name}")
    endif()
    
    target_compile_definitions(${target_name} PRIVATE
        $<$<CONFIG:Debug>:YUCHEN_DEBUG=1;DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG>
    )
    
    yuchen_log("Configured compiler for ${target_name}")
endfunction()