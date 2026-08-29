function(enable_compiler_warnings target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Target '${target}' does not exist"
        )
    endif()

    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
    )
endfunction()