find_package(Catch2 CONFIG REQUIRED)

add_executable(trading_net_udp_test
    endpoint_test.cpp
    udp_socket_test.cpp
    udp_channel_test.cpp
)

target_link_libraries(trading_net_udp_test
    PRIVATE
        trading_net_udp
        Catch2::Catch2WithMain
)

enable_compiler_warnings(trading_net_udp_test)

if(ENABLE_ASAN)
    enable_asan(trading_net_udp_test)
endif()

if(ENABLE_TSAN)
    enable_tsan(trading_net_udp_test)
endif()

if(ENABLE_STATIC_ANALYSIS)
    enable_static_analysis(trading_net_udp_test)
endif()

include(Catch)

catch_discover_tests(trading_net_udp_test)