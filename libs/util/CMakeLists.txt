find_package(Threads REQUIRED)

add_library(trading_util
    src/thread.cpp
)

target_include_directories(trading_util
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_compiler_warnings(trading_util)

target_compile_definitions(trading_util PRIVATE _GNU_SOURCE)

target_link_libraries(trading_util PUBLIC Threads::Threads)

if(ENABLE_ASAN)
    enable_asan(trading_util)
endif()

if(ENABLE_TSAN)
    enable_tsan(trading_util)
endif()

if(ENABLE_STATIC_ANALYSIS)
    enable_static_analysis(trading_util)
endif()
