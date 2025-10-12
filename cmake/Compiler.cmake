function(yuchen_configure_target target_name)
    if(APPLE)
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -pedantic
            $<$<CONFIG:Debug>:-g3 -O0>
            $<$<CONFIG:Release>:-O3>
            $<$<BOOL:${YUCHEN_WARNINGS_AS_ERRORS}>:-Werror>
        )
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /utf-8
            /Zc:__cplusplus
            /wd4100
            /wd4819
            /wd4996
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2>
            $<$<BOOL:${YUCHEN_WARNINGS_AS_ERRORS}>:/WX>
        )
        
        target_compile_definitions(${target_name} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            UNICODE
            _UNICODE
        )
    endif()
    
    target_compile_definitions(${target_name} PRIVATE
        $<$<CONFIG:Debug>:YUCHEN_DEBUG=1;DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG>
    )
endfunction()