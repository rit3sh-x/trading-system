find_package(Catch2 CONFIG REQUIRED)

add_executable(trading_util_test
    thread_test.cpp
)

target_link_libraries(trading_util_test
    PRIVATE
        trading_util
        Catch2::Catch2WithMain
)

enable_compiler_warnings(trading_util_test)

target_compile_definitions(trading_util_test PRIVATE _GNU_SOURCE)

if(ENABLE_ASAN)
    enable_asan(trading_util_test)
endif()

if(ENABLE_TSAN)
    enable_tsan(trading_util_test)
endif()

if(ENABLE_STATIC_ANALYSIS)
    enable_static_analysis(trading_util_test)
endif()

include(Catch)

catch_discover_tests(trading_util_test)
