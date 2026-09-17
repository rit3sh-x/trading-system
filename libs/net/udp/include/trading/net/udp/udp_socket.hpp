#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>

#include <trading/net/udp/endpoint.hpp>
#include <trading/net/udp/statistics.hpp>

namespace trading::net::udp {

// Errors that can occur while working with a UDP socket.
enum class UdpError : std::uint8_t {
    SocketCreationFailed,
    InvalidAddress,

    AddressAlreadyInUse,
    AlreadyBound,
    BindFailed,

    SendFailed,
    MessageTooLarge,

    ReceiveFailed,
    DatagramTruncated,
    WouldBlock,

    MulticastJoinFailed,
    MulticastLeaveFailed,

    SystemError
};

// Information about a received UDP datagram.
struct ReceiveResult {
    // Number of bytes written to the receive buffer.
    std::size_t bytes_received;

    // Source endpoint of the datagram, if available.
    std::optional<Endpoint> sender;

    // Indicates that the datagram was larger than the receive buffer.
    bool truncated;
};

class UdpSocket {
public:
    UdpSocket() = default;
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // Transfers ownership of the underlying socket.
    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;

    // Binds the socket to the specified local endpoint.
    //
    // Returns an error if the socket is already bound or the bind fails.
    [[nodiscard]]
    std::expected<void, UdpError>
    bind(const Endpoint& endpoint);

    // Sends a UDP datagram to the specified destination.
    //
    // Returns the number of bytes sent.
    [[nodiscard]]
    std::expected<std::size_t, UdpError>
    send_to(
        std::span<const std::byte> data,
        const Endpoint& destination
    );

    // Receives a single UDP datagram into the provided buffer.
    //
    // Returns WouldBlock when the socket is non-blocking and no datagram is currently available.
    [[nodiscard]]
    std::expected<ReceiveResult, UdpError>
    receive_from(std::span<std::byte> buffer);

    // Receives multiple UDP datagrams into the provided buffer.
    //
    // Each datagram occupies datagram_capacity bytes in the buffer. Returns the number of datagrams received.
    [[nodiscard]]
    std::expected<std::size_t, UdpError>
    receive_many(
        std::span<std::byte> buffer,
        std::size_t datagram_capacity,
        std::span<ReceiveResult> results
    );

    // Enables or disables non-blocking I/O.
    [[nodiscard]]
    std::expected<void, UdpError>
    set_non_blocking(bool enabled);

    // Sets the kernel receive buffer size.
    [[nodiscard]]
    std::expected<void, UdpError>
    set_receive_buffer(std::size_t size);

    // Sets the kernel send buffer size.
    [[nodiscard]]
    std::expected<void, UdpError>
    set_send_buffer(std::size_t size);

    // Returns whether the socket is currently open.
    [[nodiscard]]
    bool is_open() const noexcept;

    // Closes the socket.
    //
    // Closing an already closed socket succeeds.
    [[nodiscard]]
    std::expected<void, UdpError>
    close() noexcept;

    // Returns the local endpoint to which the socket is bound.
    [[nodiscard]]
    std::expected<Endpoint, UdpError>
    local_endpoint() const;

    // Joins an IPv4 multicast group.
    [[nodiscard]]
    std::expected<void, UdpError>
    join_multicast_group(const Endpoint& multicast_endpoint);

    // Leaves an IPv4 multicast group.
    [[nodiscard]]
    std::expected<void, UdpError>
    leave_multicast_group(const Endpoint& multicast_endpoint);

    // Sets the network interface used for multicast traffic.
    [[nodiscard]]
    std::expected<void, UdpError>
    set_multicast_interface(const Endpoint& interface);

    // Enables or disables multicast loopback.
    [[nodiscard]]
    std::expected<void, UdpError>
    set_multicast_loopback(bool enabled);

    // Returns a snapshot of the socket's current statistics.
    //
    // Statistics are safe to read concurrently with socket operations.
    [[nodiscard]]
    UdpStatistics statistics() const noexcept;

private:
    int socket_{-1};
    UdpStatistics statistics_;
};

}