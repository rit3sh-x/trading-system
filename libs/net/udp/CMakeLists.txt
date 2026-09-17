add_library(trading_net_udp
    src/endpoint.cpp
    src/statistics.cpp
    src/udp_socket.cpp
    src/udp_channel.cpp
)

target_include_directories(trading_net_udp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_compiler_warnings(trading_net_udp)

target_compile_definitions(trading_net_udp PRIVATE _GNU_SOURCE)

if(ENABLE_ASAN)
    enable_asan(trading_net_udp)
endif()

if(ENABLE_TSAN)
    enable_tsan(trading_net_udp)
endif()

if(ENABLE_STATIC_ANALYSIS)
    enable_static_analysis(trading_net_udp)
endif()