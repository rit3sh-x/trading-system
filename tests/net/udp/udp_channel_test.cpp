#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <thread>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <trading/net/udp/endpoint.hpp>
#include <trading/net/udp/udp_channel.hpp>
#include <trading/net/udp/udp_socket.hpp>

using namespace trading::net::udp;

TEST_CASE("UdpChannel creates from socket", "[udp_channel]") {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    UdpChannel channel(std::move(socket));

    REQUIRE(channel.is_open());
}

TEST_CASE("UdpChannel sends and receives data", "[udp_channel]") {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    UdpChannel channel(std::move(socket));

    constexpr std::array<std::byte, 5> message{
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}
    };

    bool received = false;
    std::array<std::byte, 5> received_data{};

    channel.set_receive_handler(
        [&](std::span<const std::byte> data,
            const Endpoint& sender) {

            received = true;

            REQUIRE(sender.address() == "127.0.0.1");
            REQUIRE(data.size() == message.size());

            std::copy(
                data.begin(),
                data.end(),
                received_data.begin()
            );
        }
    );

    auto send_result = channel.send(message, *local);

    REQUIRE(send_result.has_value());
    REQUIRE(*send_result == message.size());

    using namespace std::chrono_literals;

    const auto deadline =
        std::chrono::steady_clock::now() + 500ms;

    while (!received &&
           std::chrono::steady_clock::now() < deadline) {

        REQUIRE(channel.poll().has_value());

        if (!received) {
            std::this_thread::sleep_for(1ms);
        }
    }

    REQUIRE(received);
    REQUIRE(received_data == message);
}

TEST_CASE("UdpChannel drains all available packets", "[udp_channel]") {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    UdpChannel channel(std::move(socket));

    constexpr std::array<std::byte, 3> message{
        std::byte{'a'},
        std::byte{'b'},
        std::byte{'c'}
    };

    constexpr std::size_t packet_count = 10;

    std::size_t received_count = 0;

    channel.set_receive_handler(
        [&](std::span<const std::byte> data,
            const Endpoint&) {

            REQUIRE(data.size() == message.size());

            ++received_count;
        }
    );

    for (std::size_t i = 0; i < packet_count; ++i) {
        auto result = channel.send(message, *local);

        REQUIRE(result.has_value());
        REQUIRE(*result == message.size());
    }

    using namespace std::chrono_literals;

    const auto deadline =
        std::chrono::steady_clock::now() + 1000ms;

    while (received_count < packet_count &&
           std::chrono::steady_clock::now() < deadline) {

        REQUIRE(channel.poll().has_value());

        if (received_count < packet_count) {
            std::this_thread::sleep_for(1ms);
        }
    }

    REQUIRE(received_count == packet_count);
}

TEST_CASE(
    "UdpChannel poll returns when no data is available",
    "[udp_channel]"
) {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    UdpChannel channel(std::move(socket));

    bool called = false;

    channel.set_receive_handler(
        [&](std::span<const std::byte>,
            const Endpoint&) {
            called = true;
        }
    );

    auto result = channel.poll();

    REQUIRE(result.has_value());
    REQUIRE_FALSE(called);
}

TEST_CASE(
    "UdpChannel invokes receive handler once per packet",
    "[udp_channel]"
) {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    UdpChannel channel(std::move(socket));

    constexpr std::array<std::byte, 4> message{
        std::byte{'t'},
        std::byte{'i'},
        std::byte{'c'},
        std::byte{'k'}
    };

    std::size_t handler_calls = 0;

    channel.set_receive_handler(
        [&](std::span<const std::byte> data,
            const Endpoint&) {

            ++handler_calls;

            REQUIRE(data.size() == message.size());
        }
    );

    REQUIRE(channel.send(message, *local).has_value());

    using namespace std::chrono_literals;

    const auto deadline =
        std::chrono::steady_clock::now() + 500ms;

    while (handler_calls == 0 &&
           std::chrono::steady_clock::now() < deadline) {

        REQUIRE(channel.poll().has_value());

        if (handler_calls == 0) {
            std::this_thread::sleep_for(1ms);
        }
    }

    REQUIRE(handler_calls == 1);
}

TEST_CASE(
    "UdpChannel works without receive handler",
    "[udp_channel]"
) {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    UdpChannel channel(std::move(socket));

    constexpr std::array<std::byte, 4> message{
        std::byte{'t'},
        std::byte{'e'},
        std::byte{'s'},
        std::byte{'t'}
    };

    REQUIRE(channel.send(message, *local).has_value());

    using namespace std::chrono_literals;

    const auto deadline =
        std::chrono::steady_clock::now() + 500ms;

    bool drained = false;

    while (!drained &&
           std::chrono::steady_clock::now() < deadline) {

        auto result = channel.poll();

        REQUIRE(result.has_value());

        std::this_thread::sleep_for(1ms);

        drained = true;
    }
}

TEST_CASE(
    "UdpChannel reports receive errors",
    "[udp_channel]"
) {
    UdpSocket socket;

    UdpChannel channel(std::move(socket));

    UdpError received_error{};
    bool error_called = false;

    channel.set_error_handler(
        [&](UdpError error) {
            received_error = error;
            error_called = true;
        }
    );

    auto result = channel.poll();

    REQUIRE_FALSE(result.has_value());
    REQUIRE(error_called);
    REQUIRE(received_error == UdpError::SocketCreationFailed);
}

TEST_CASE(
    "UdpChannel reports send errors",
    "[udp_channel]"
) {
    UdpSocket socket;

    UdpChannel channel(std::move(socket));

    constexpr std::array<std::byte, 4> message{
        std::byte{'t'},
        std::byte{'e'},
        std::byte{'s'},
        std::byte{'t'}
    };

    auto destination =
        Endpoint::create("127.0.0.1", 9999);

    REQUIRE(destination.has_value());

    auto result =
        channel.send(message, *destination);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(
        result.error() == UdpError::SocketCreationFailed
    );
}

TEST_CASE(
    "UdpChannel move constructor transfers socket ownership",
    "[udp_channel]"
) {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    UdpChannel channel(std::move(socket));

    UdpChannel moved_channel(std::move(channel));

    REQUIRE_FALSE(channel.is_open());
    REQUIRE(moved_channel.is_open());
}

TEST_CASE(
    "UdpChannel move assignment transfers socket ownership",
    "[udp_channel]"
) {
    auto endpoint1 = Endpoint::create("127.0.0.1", 0);
    auto endpoint2 = Endpoint::create("127.0.0.1", 0);

    REQUIRE(endpoint1.has_value());
    REQUIRE(endpoint2.has_value());

    UdpSocket socket1;
    UdpSocket socket2;

    REQUIRE(socket1.bind(*endpoint1).has_value());
    REQUIRE(socket2.bind(*endpoint2).has_value());

    REQUIRE(socket1.set_non_blocking(true).has_value());
    REQUIRE(socket2.set_non_blocking(true).has_value());

    UdpChannel channel1(std::move(socket1));
    UdpChannel channel2(std::move(socket2));

    channel2 = std::move(channel1);

    REQUIRE_FALSE(channel1.is_open());
    REQUIRE(channel2.is_open());
}

TEST_CASE(
    "UdpChannel drains bursts larger than one batch",
    "[udp_channel]"
) {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    UdpChannel channel(std::move(socket));

    constexpr std::array<std::byte, 3> message{
        std::byte{'x'},
        std::byte{'y'},
        std::byte{'z'}
    };

    constexpr std::size_t packet_count = 130;

    std::size_t received_count = 0;

    channel.set_receive_handler(
        [&](std::span<const std::byte> data,
            const Endpoint&) {

            REQUIRE(data.size() == message.size());

            ++received_count;
        }
    );

    for (std::size_t i = 0; i < packet_count; ++i) {
        auto result = channel.send(message, *local);

        REQUIRE(result.has_value());
        REQUIRE(*result == message.size());
    }

    using namespace std::chrono_literals;

    const auto deadline =
        std::chrono::steady_clock::now() + 1000ms;

    while (received_count < packet_count &&
           std::chrono::steady_clock::now() < deadline) {

        REQUIRE(channel.poll().has_value());

        if (received_count < packet_count) {
            std::this_thread::sleep_for(1ms);
        }
    }

    REQUIRE(received_count == packet_count);
}

TEST_CASE(
    "UdpChannel reports oversized datagrams as truncated",
    "[udp_channel]"
) {
    auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());

    UdpSocket socket;

    REQUIRE(socket.bind(*endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    UdpChannel channel(std::move(socket));

    constexpr std::size_t oversized_size = 3000;

    const std::vector<std::byte> oversized(
        oversized_size,
        std::byte{'z'}
    );

    bool handler_called = false;
    bool error_called = false;
    UdpError reported_error{};

    channel.set_receive_handler(
        [&](std::span<const std::byte>,
            const Endpoint&) {
            handler_called = true;
        }
    );

    channel.set_error_handler(
        [&](UdpError error) {
            reported_error = error;
            error_called = true;
        }
    );

    auto send_result =
        channel.send(oversized, *local);

    REQUIRE(send_result.has_value());
    REQUIRE(*send_result == oversized.size());

    using namespace std::chrono_literals;

    const auto deadline =
        std::chrono::steady_clock::now() + 500ms;

    while (!error_called &&
           std::chrono::steady_clock::now() < deadline) {

        REQUIRE(channel.poll().has_value());

        if (!error_called) {
            std::this_thread::sleep_for(1ms);
        }
    }

    CAPTURE(handler_called);
    CAPTURE(error_called);
    CAPTURE(reported_error);
    REQUIRE(error_called);
    REQUIRE(
        reported_error == UdpError::DatagramTruncated
    );
    REQUIRE_FALSE(handler_called);
}
