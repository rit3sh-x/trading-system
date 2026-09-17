#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <string_view>
#include <thread>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <trading/net/udp/endpoint.hpp>
#include <trading/net/udp/udp_socket.hpp>

using namespace trading::net::udp;

namespace {

Endpoint localhost() {
    const auto endpoint = Endpoint::create("127.0.0.1", 0);
    REQUIRE(endpoint.has_value());
    return *endpoint;
}

Endpoint any_address() {
    const auto endpoint = Endpoint::create("0.0.0.0", 0);
    REQUIRE(endpoint.has_value());
    return *endpoint;
}

Endpoint multicast_address() {
    const auto endpoint = Endpoint::create("239.255.0.1", 9999);
    REQUIRE(endpoint.has_value());
    return *endpoint;
}

}

TEST_CASE("UdpSocket starts closed", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE_FALSE(socket.is_open());
}

TEST_CASE("UdpSocket binds IPv4 endpoint", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());
    REQUIRE(socket.is_open());
}

TEST_CASE("UdpSocket prevents rebinding", "[udp_socket]") {
    const auto endpoint1 = localhost();
    const auto endpoint2 = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint1).has_value());

    const auto result = socket.bind(endpoint2);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == UdpError::AlreadyBound);
}

TEST_CASE("UdpSocket can be closed", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());
    REQUIRE(socket.is_open());

    REQUIRE(socket.close().has_value());
    REQUIRE_FALSE(socket.is_open());
}

TEST_CASE("Closing an already closed socket succeeds", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.close().has_value());
    REQUIRE_FALSE(socket.is_open());
}

TEST_CASE("Closing socket twice succeeds", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    REQUIRE(socket.close().has_value());
    REQUIRE(socket.close().has_value());

    REQUIRE_FALSE(socket.is_open());
}

TEST_CASE("Operations on closed socket fail", "[udp_socket]") {
    UdpSocket socket;

    const auto endpoint = localhost();
    std::array<std::byte, 8> buffer{};

    const auto send = socket.send_to(buffer, endpoint);
    REQUIRE_FALSE(send.has_value());
    REQUIRE(send.error() == UdpError::SocketCreationFailed);

    const auto receive = socket.receive_from(buffer);
    REQUIRE_FALSE(receive.has_value());
    REQUIRE(receive.error() == UdpError::SocketCreationFailed);

    const auto receive_many = socket.receive_many(
        buffer,
        8,
        std::span<ReceiveResult>{}
    );
    REQUIRE_FALSE(receive_many.has_value());
    REQUIRE(receive_many.error() == UdpError::SocketCreationFailed);

    const auto non_blocking = socket.set_non_blocking(true);
    REQUIRE_FALSE(non_blocking.has_value());
    REQUIRE(non_blocking.error() == UdpError::SocketCreationFailed);

    const auto receive_buffer = socket.set_receive_buffer(1024);
    REQUIRE_FALSE(receive_buffer.has_value());
    REQUIRE(receive_buffer.error() == UdpError::SocketCreationFailed);

    const auto send_buffer = socket.set_send_buffer(1024);
    REQUIRE_FALSE(send_buffer.has_value());
    REQUIRE(send_buffer.error() == UdpError::SocketCreationFailed);

    const auto local = socket.local_endpoint();
    REQUIRE_FALSE(local.has_value());
    REQUIRE(local.error() == UdpError::SocketCreationFailed);

    const auto multicast = multicast_address();

    const auto join = socket.join_multicast_group(multicast);
    REQUIRE_FALSE(join.has_value());
    REQUIRE(join.error() == UdpError::SocketCreationFailed);

    const auto leave = socket.leave_multicast_group(multicast);
    REQUIRE_FALSE(leave.has_value());
    REQUIRE(leave.error() == UdpError::SocketCreationFailed);

    const auto interface = socket.set_multicast_interface(endpoint);
    REQUIRE_FALSE(interface.has_value());
    REQUIRE(interface.error() == UdpError::SocketCreationFailed);

    const auto loopback = socket.set_multicast_loopback(true);
    REQUIRE_FALSE(loopback.has_value());
    REQUIRE(loopback.error() == UdpError::SocketCreationFailed);
}

TEST_CASE("UdpSocket reports local endpoint", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    const auto local = socket.local_endpoint();

    REQUIRE(local.has_value());
    REQUIRE(local->address() == "127.0.0.1");
    REQUIRE(local->port() != 0);
}

TEST_CASE("UdpSocket send and receive", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    constexpr std::array<std::byte, 5> message{
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}
    };

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    const auto send_result =
        socket.send_to(message, *local);

    REQUIRE(send_result.has_value());
    REQUIRE(*send_result == message.size());

    std::array<std::byte, 64> buffer{};

    const auto receive_result =
        socket.receive_from(buffer);

    REQUIRE(receive_result.has_value());
    REQUIRE(
        receive_result->bytes_received == message.size()
    );
    REQUIRE(receive_result->sender.has_value());
    REQUIRE(receive_result->sender->address() == "127.0.0.1");
    REQUIRE(receive_result->sender->port() != 0);
    REQUIRE_FALSE(receive_result->truncated);

    REQUIRE(std::equal(
        message.begin(),
        message.end(),
        buffer.begin()
    ));
}

TEST_CASE("UdpSocket receives empty datagram", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    std::span<const std::byte> empty{};

    const auto send_result = socket.send_to(empty, *local);

    REQUIRE(send_result.has_value());
    REQUIRE(*send_result == 0);

    std::array<std::byte, 64> buffer{};

    const auto receive_result = socket.receive_from(buffer);

    REQUIRE(receive_result.has_value());
    REQUIRE(receive_result->bytes_received == 0);
    REQUIRE(receive_result->sender.has_value());
    REQUIRE_FALSE(receive_result->truncated);
}

TEST_CASE("Non-blocking socket returns WouldBlock", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    std::array<std::byte, 1024> buffer{};

    const auto result = socket.receive_from(buffer);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == UdpError::WouldBlock);
}

TEST_CASE(
    "Non-blocking receive_many returns WouldBlock when empty",
    "[udp_socket]"
) {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    constexpr std::size_t datagram_capacity = 64;

    std::array<std::byte, 256> buffer{};
    std::array<ReceiveResult, 4> results{};

    const auto result = socket.receive_many(
        buffer,
        datagram_capacity,
        results
    );

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == UdpError::WouldBlock);
}

TEST_CASE(
    "receive_many receives multiple datagrams",
    "[udp_socket]"
) {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::size_t datagram_capacity = 64;

    constexpr std::array<std::byte, 5> first{
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}
    };

    constexpr std::array<std::byte, 5> second{
        std::byte{'w'},
        std::byte{'o'},
        std::byte{'r'},
        std::byte{'l'},
        std::byte{'d'}
    };

    constexpr std::array<std::byte, 3> third{
        std::byte{'u'},
        std::byte{'d'},
        std::byte{'p'}
    };

    REQUIRE(socket.send_to(first, *local).has_value());
    REQUIRE(socket.send_to(second, *local).has_value());
    REQUIRE(socket.send_to(third, *local).has_value());

    std::array<std::byte, datagram_capacity * 4> buffer{};
    std::array<ReceiveResult, 4> results{};

    const auto result = socket.receive_many(
        buffer,
        datagram_capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 3);

    REQUIRE(results[0].bytes_received == first.size());
    REQUIRE(results[1].bytes_received == second.size());
    REQUIRE(results[2].bytes_received == third.size());

    REQUIRE(results[0].sender.has_value());
    REQUIRE(results[1].sender.has_value());
    REQUIRE(results[2].sender.has_value());

    REQUIRE_FALSE(results[0].truncated);
    REQUIRE_FALSE(results[1].truncated);
    REQUIRE_FALSE(results[2].truncated);

    REQUIRE(std::equal(
        first.begin(),
        first.end(),
        buffer.begin()
    ));

    REQUIRE(std::equal(
        second.begin(),
        second.end(),
        buffer.begin() + datagram_capacity
    ));

    REQUIRE(std::equal(
        third.begin(),
        third.end(),
        buffer.begin() + (2 * datagram_capacity)
    ));
}

TEST_CASE("receive_many respects result capacity", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::size_t datagram_capacity = 64;

    constexpr std::array<std::byte, 1> message{
        std::byte{'x'}
    };

    for (int i = 0; i < 3; ++i) {
        REQUIRE(socket.send_to(message, *local).has_value());
    }

    std::array<std::byte, datagram_capacity * 2> buffer{};
    std::array<ReceiveResult, 2> results{};

    const auto result = socket.receive_many(
        buffer,
        datagram_capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 2);

    const auto next = socket.receive_many(
        buffer,
        datagram_capacity,
        results
    );

    REQUIRE(next.has_value());
    REQUIRE(*next == 1);
}

TEST_CASE("receive_many reports truncated datagrams", "[udp_socket]") {
    const auto endpoint = localhost();

    UdpSocket socket;

    REQUIRE(socket.bind(endpoint).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::size_t datagram_capacity = 2048;

    std::array<std::byte, datagram_capacity + 1> message{};

    REQUIRE(socket.send_to(message, *local).has_value());

    std::array<std::byte, datagram_capacity> buffer{};
    std::array<ReceiveResult, 1> results{};

    const auto result = socket.receive_many(
        buffer,
        datagram_capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 1);
    REQUIRE(results[0].truncated);
    REQUIRE(results[0].bytes_received == message.size());
}

TEST_CASE("receive_many with zero results does nothing", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    std::array<std::byte, 8> buffer{};
    std::span<ReceiveResult> results{};

    const auto received = socket.receive_many(
        buffer,
        8,
        results
    );

    REQUIRE(received.has_value());
    REQUIRE(*received == 0);
}

TEST_CASE("receive_many with zero datagram capacity does nothing", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    std::array<std::byte, 64> buffer{};
    std::array<ReceiveResult, 2> results{};

    const auto result = socket.receive_many(
        buffer,
        0,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 0);
}

TEST_CASE("receive_many rejects insufficient buffer", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    std::array<std::byte, 63> buffer{};
    std::array<ReceiveResult, 2> results{};

    const auto result = socket.receive_many(
        buffer,
        64,
        results
    );

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == UdpError::SystemError);
}

TEST_CASE(
    "receive_many rejects insufficient buffer for requested slots",
    "[udp_socket]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    std::array<std::byte, 128> buffer{};
    std::array<ReceiveResult, 4> results{};

    const auto result = socket.receive_many(
        buffer,
        64,
        results
    );

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == UdpError::SystemError);
}

TEST_CASE("UdpSocket sets non-blocking mode", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    REQUIRE(socket.set_non_blocking(true).has_value());

    std::array<std::byte, 64> buffer{};

    const auto blocked = socket.receive_from(buffer);

    REQUIRE_FALSE(blocked.has_value());
    REQUIRE(blocked.error() == UdpError::WouldBlock);

    REQUIRE(socket.set_non_blocking(false).has_value());
}

TEST_CASE("UdpSocket can toggle non-blocking mode", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    REQUIRE(socket.set_non_blocking(true).has_value());
    REQUIRE(socket.set_non_blocking(false).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());
    REQUIRE(socket.set_non_blocking(false).has_value());
}

TEST_CASE("UdpSocket sets receive buffer", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());
    REQUIRE(socket.set_receive_buffer(1 << 20).has_value());
}

TEST_CASE("UdpSocket sets send buffer", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());
    REQUIRE(socket.set_send_buffer(1 << 20).has_value());
}

TEST_CASE("UdpSocket accepts zero receive buffer size", "[udp_socket]") {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());
    REQUIRE(socket.set_receive_buffer(0).has_value());
}

TEST_CASE(
    "UdpSocket accepts zero send buffer size",
    "[udp_socket]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());
    REQUIRE(socket.set_send_buffer(0).has_value());
}

TEST_CASE(
    "UdpSocket rejects oversized socket buffer",
    "[udp_socket]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    constexpr auto oversized =
        static_cast<std::size_t>(INT_MAX) + 1;

    const auto receive = socket.set_receive_buffer(oversized);
    REQUIRE_FALSE(receive.has_value());
    REQUIRE(receive.error() == UdpError::SystemError);

    const auto send = socket.set_send_buffer(oversized);
    REQUIRE_FALSE(send.has_value());
    REQUIRE(send.error() == UdpError::SystemError);
}

TEST_CASE(
    "UdpSocket move constructor transfers ownership",
    "[udp_socket][move]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    UdpSocket moved_socket(std::move(socket));

    REQUIRE_FALSE(socket.is_open());
    REQUIRE(moved_socket.is_open());

    REQUIRE(moved_socket.local_endpoint().has_value());
}

TEST_CASE(
    "UdpSocket move constructor preserves statistics",
    "[udp_socket][move][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::array<std::byte, 3> message{
        std::byte{'u'},
        std::byte{'d'},
        std::byte{'p'}
    };

    REQUIRE(socket.send_to(message, *local).has_value());

    std::array<std::byte, 64> buffer{};
    REQUIRE(socket.receive_from(buffer).has_value());

    const auto before = socket.statistics();

    UdpSocket moved_socket(std::move(socket));

    const auto after = moved_socket.statistics();

    REQUIRE(after.datagrams_sent() == before.datagrams_sent());
    REQUIRE(after.bytes_sent() == before.bytes_sent());
    REQUIRE(after.datagrams_received() == before.datagrams_received());
    REQUIRE(after.bytes_received() == before.bytes_received());
    REQUIRE(after.datagrams_truncated() == before.datagrams_truncated());
}

TEST_CASE(
    "UdpSocket move assignment transfers ownership",
    "[udp_socket][move]"
) {
    UdpSocket socket1;
    UdpSocket socket2;

    REQUIRE(socket1.bind(localhost()).has_value());
    REQUIRE(socket2.bind(localhost()).has_value());

    socket2 = std::move(socket1);

    REQUIRE_FALSE(socket1.is_open());
    REQUIRE(socket2.is_open());

    REQUIRE(socket2.local_endpoint().has_value());
}

TEST_CASE(
    "UdpSocket move assignment preserves statistics",
    "[udp_socket][move][statistics]"
) {
    UdpSocket socket1;
    UdpSocket socket2;

    REQUIRE(socket1.bind(localhost()).has_value());
    REQUIRE(socket2.bind(localhost()).has_value());

    const auto local = socket1.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::array<std::byte, 3> message{
        std::byte{'u'},
        std::byte{'d'},
        std::byte{'p'}
    };

    REQUIRE(socket1.send_to(message, *local).has_value());

    std::array<std::byte, 64> buffer{};
    REQUIRE(socket1.receive_from(buffer).has_value());

    const auto before = socket1.statistics();

    socket2 = std::move(socket1);

    const auto after = socket2.statistics();

    REQUIRE(after.datagrams_sent() == before.datagrams_sent());
    REQUIRE(after.bytes_sent() == before.bytes_sent());
    REQUIRE(after.datagrams_received() == before.datagrams_received());
    REQUIRE(after.bytes_received() == before.bytes_received());
}

TEST_CASE(
    "UdpSocket move assignment closes previous socket",
    "[udp_socket][move]"
) {
    UdpSocket socket1;
    UdpSocket socket2;

    REQUIRE(socket1.bind(localhost()).has_value());
    REQUIRE(socket2.bind(localhost()).has_value());

    const auto old_endpoint = socket2.local_endpoint();
    REQUIRE(old_endpoint.has_value());

    socket2 = std::move(socket1);

    REQUIRE_FALSE(socket1.is_open());
    REQUIRE(socket2.is_open());

    const auto new_endpoint = socket2.local_endpoint();
    REQUIRE(new_endpoint.has_value());

    REQUIRE(new_endpoint->port() != old_endpoint->port());
}

TEST_CASE(
    "UdpSocket statistics start at zero",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    const auto statistics = socket.statistics();

    REQUIRE(statistics.datagrams_received() == 0);
    REQUIRE(statistics.bytes_received() == 0);
    REQUIRE(statistics.datagrams_sent() == 0);
    REQUIRE(statistics.bytes_sent() == 0);
    REQUIRE(statistics.datagrams_truncated() == 0);
    REQUIRE(statistics.send_errors() == 0);
    REQUIRE(statistics.receive_errors() == 0);
    REQUIRE(statistics.would_block() == 0);
    REQUIRE(statistics.batches_received() == 0);
    REQUIRE(statistics.max_batch_size() == 0);
    REQUIRE(statistics.kernel_drops() == 0);
}

TEST_CASE(
    "UdpSocket statistics track sent datagrams",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::array<std::byte, 5> message{
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}
    };

    REQUIRE(socket.send_to(message, *local).has_value());
    REQUIRE(socket.send_to(message, *local).has_value());

    const auto statistics = socket.statistics();

    REQUIRE(statistics.datagrams_sent() == 2);
    REQUIRE(statistics.bytes_sent() == 10);
    REQUIRE(statistics.send_errors() == 0);
}

TEST_CASE(
    "UdpSocket statistics track received datagrams",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::array<std::byte, 5> message{
        std::byte{'h'},
        std::byte{'e'},
        std::byte{'l'},
        std::byte{'l'},
        std::byte{'o'}
    };

    REQUIRE(socket.send_to(message, *local).has_value());
    REQUIRE(socket.send_to(message, *local).has_value());

    std::array<std::byte, 64> buffer{};

    REQUIRE(socket.receive_from(buffer).has_value());
    REQUIRE(socket.receive_from(buffer).has_value());

    const auto statistics = socket.statistics();

    REQUIRE(statistics.datagrams_received() == 2);
    REQUIRE(statistics.bytes_received() == 10);
    REQUIRE(statistics.datagrams_truncated() == 0);
}

TEST_CASE(
    "UdpSocket statistics track WouldBlock",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());
    REQUIRE(socket.set_non_blocking(true).has_value());

    std::array<std::byte, 64> buffer{};

    const auto result = socket.receive_from(buffer);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == UdpError::WouldBlock);

    const auto statistics = socket.statistics();

    REQUIRE(statistics.would_block() == 1);
    REQUIRE(statistics.receive_errors() == 0);
}

TEST_CASE(
    "UdpSocket statistics track batched receives",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::size_t datagram_capacity = 64;
    constexpr std::size_t count = 4;

    constexpr std::array<std::byte, 3> message{
        std::byte{'u'},
        std::byte{'d'},
        std::byte{'p'}
    };

    for (std::size_t i = 0; i < count; ++i) {
        REQUIRE(socket.send_to(message, *local).has_value());
    }

    std::array<std::byte, datagram_capacity * count> buffer{};
    std::array<ReceiveResult, count> results{};

    const auto result = socket.receive_many(
        buffer,
        datagram_capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == count);

    const auto statistics = socket.statistics();

    REQUIRE(statistics.datagrams_sent() == count);
    REQUIRE(statistics.bytes_sent() == count * message.size());
    REQUIRE(statistics.datagrams_received() == count);
    REQUIRE(statistics.bytes_received() == count * message.size());
    REQUIRE(statistics.batches_received() == 1);
    REQUIRE(statistics.max_batch_size() == count);
}

TEST_CASE(
    "UdpSocket statistics track truncated datagrams",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::size_t capacity = 64;

    std::array<std::byte, capacity + 1> message{};

    REQUIRE(socket.send_to(message, *local).has_value());

    std::array<std::byte, capacity> buffer{};
    std::array<ReceiveResult, 1> results{};

    const auto result = socket.receive_many(
        buffer,
        capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 1);
    REQUIRE(results[0].truncated);

    const auto statistics = socket.statistics();

    REQUIRE(statistics.datagrams_received() == 1);
    REQUIRE(statistics.datagrams_truncated() == 1);
}

TEST_CASE(
    "UdpSocket statistics track maximum batch size",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::size_t capacity = 64;

    constexpr std::array<std::byte, 1> message{
        std::byte{'x'}
    };

    for (int i = 0; i < 3; ++i) {
        REQUIRE(socket.send_to(message, *local).has_value());
    }

    std::array<std::byte, capacity * 3> buffer{};
    std::array<ReceiveResult, 3> results{};

    auto result = socket.receive_many(
        buffer,
        capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 3);

    for (int i = 0; i < 2; ++i) {
        REQUIRE(socket.send_to(message, *local).has_value());
    }

    result = socket.receive_many(
        buffer,
        capacity,
        results
    );

    REQUIRE(result.has_value());
    REQUIRE(*result == 2);

    const auto statistics = socket.statistics();

    REQUIRE(statistics.batches_received() == 2);
    REQUIRE(statistics.max_batch_size() == 3);
}

TEST_CASE(
    "UdpSocket statistics accumulate across calls",
    "[udp_socket][statistics]"
) {
    UdpSocket socket;

    REQUIRE(socket.bind(localhost()).has_value());

    const auto local = socket.local_endpoint();
    REQUIRE(local.has_value());

    constexpr std::array<std::byte, 4> message{
        std::byte{'t'},
        std::byte{'e'},
        std::byte{'s'},
        std::byte{'t'}
    };

    std::array<std::byte, 64> buffer{};

    REQUIRE(socket.send_to(message, *local).has_value());
    REQUIRE(socket.receive_from(buffer).has_value());

    REQUIRE(socket.send_to(message, *local).has_value());
    REQUIRE(socket.receive_from(buffer).has_value());

    const auto statistics = socket.statistics();

    REQUIRE(statistics.datagrams_sent() == 2);
    REQUIRE(statistics.bytes_sent() == 8);
    REQUIRE(statistics.datagrams_received() == 2);
    REQUIRE(statistics.bytes_received() == 8);
}

TEST_CASE("UdpSocket joins multicast group", "[udp_socket][multicast]") {
    UdpSocket socket;

    REQUIRE(socket.bind(any_address()).has_value());

    const auto result = socket.join_multicast_group(multicast_address());

    REQUIRE(result.has_value());
}

TEST_CASE("UdpSocket leaves multicast group", "[udp_socket][multicast]") {
    UdpSocket socket;

    REQUIRE(socket.bind(any_address()).has_value());

    const auto multicast = multicast_address();

    REQUIRE(socket.join_multicast_group(multicast).has_value());
    REQUIRE(socket.leave_multicast_group(multicast).has_value());
}

TEST_CASE("UdpSocket sets multicast interface", "[udp_socket][multicast]") {
    UdpSocket socket;

    REQUIRE(socket.bind(any_address()).has_value());

    const auto interface = Endpoint::create("127.0.0.1", 0);

    REQUIRE(interface.has_value());

    REQUIRE(socket.set_multicast_interface(*interface).has_value());
}

TEST_CASE("UdpSocket sets multicast loopback", "[udp_socket][multicast]") {
    UdpSocket socket;

    REQUIRE(socket.bind(any_address()).has_value());

    REQUIRE(socket.set_multicast_loopback(false).has_value());

    REQUIRE(socket.set_multicast_loopback(true).has_value());
}