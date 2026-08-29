function(enable_asan target)
    target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    target_link_options(${target} PRIVATE
        -fsanitize=address,undefined
    )
endfunction()

function(enable_tsan target)
    target_compile_options(${target} PRIVATE
        -fsanitize=thread
        -fno-omit-frame-pointer
    )

    target_link_options(${target} PRIVATE
        -fsanitize=thread
    )
endfunction()