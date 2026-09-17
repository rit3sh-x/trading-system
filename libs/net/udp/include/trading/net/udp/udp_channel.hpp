#pragma once

#include <array>
#include <cstddef>
#include <expected>
#include <functional>
#include <span>

#include <trading/net/udp/endpoint.hpp>
#include <trading/net/udp/udp_socket.hpp>

namespace trading::net::udp {

using ReceiveHandler = std::function<void(
    std::span<const std::byte>,
    const Endpoint&
)>;

using ErrorHandler = std::function<void(UdpError)>;

class UdpChannel {
public:
    explicit UdpChannel(UdpSocket socket) noexcept;

    ~UdpChannel() = default;

    UdpChannel(const UdpChannel&) = delete;
    UdpChannel& operator=(const UdpChannel&) = delete;

    UdpChannel(UdpChannel&&) noexcept = default;
    UdpChannel& operator=(UdpChannel&&) noexcept = default;

    [[nodiscard]]
    std::expected<std::size_t, UdpError>
    send(
        std::span<const std::byte> data,
        const Endpoint& destination
    );

    [[nodiscard]]
    std::expected<void, UdpError>
    poll();

    void set_receive_handler(ReceiveHandler handler);
    void set_error_handler(ErrorHandler handler);

    [[nodiscard]]
    bool is_open() const noexcept;

private:
    static constexpr std::size_t batch_size = 64;
    static constexpr std::size_t max_datagram_size = 2048;

    UdpSocket socket_;

    ReceiveHandler receive_handler_;
    ErrorHandler error_handler_;

    std::array<std::byte, batch_size * max_datagram_size> receive_buffer_{};
    std::array<ReceiveResult, batch_size> receive_results_{};
};

}