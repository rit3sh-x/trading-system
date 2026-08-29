find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
find_program(CPPCHECK_EXECUTABLE cppcheck)

function(enable_static_analysis target)
    if(CLANG_TIDY_EXECUTABLE)
        set_target_properties(${target}
            PROPERTIES
                CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}"
        )
    else()
        message(WARNING "clang-tidy not found")
    endif()

    if(CPPCHECK_EXECUTABLE)
        set_target_properties(${target}
            PROPERTIES
                CXX_CPPCHECK "${CPPCHECK_EXECUTABLE}"
        )
    else()
        message(WARNING "cppcheck not found")
    endif()
endfunction()