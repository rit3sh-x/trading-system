option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable clang-tidy and cppcheck" OFF)
option(ENABLE_BENCHMARKS "Build benchmarks" ON)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR
        "ENABLE_ASAN and ENABLE_TSAN cannot be enabled together"
    )
endif()

if(ENABLE_ASAN)
    include(Sanitizers)
    enable_asan()
endif()

if(ENABLE_TSAN)
    include(Sanitizers)
    enable_tsan()
endif()

if(ENABLE_STATIC_ANALYSIS)
    include(StaticAnalysis)
endif()