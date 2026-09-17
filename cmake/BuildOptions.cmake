option(ENABLE_ASAN
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

option(ENABLE_TSAN
    "Enable ThreadSanitizer"
    OFF
)

option(ENABLE_STATIC_ANALYSIS
    "Enable clang-tidy and cppcheck"
    OFF
)

option(ENABLE_BENCHMARKS
    "Build benchmarks"
    ON
)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR
        "ENABLE_ASAN and ENABLE_TSAN cannot be enabled together"
    )
endif()