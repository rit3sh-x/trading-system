#include <utility>

#include <trading/net/udp/udp_channel.hpp>

namespace trading::net::udp {

UdpChannel::UdpChannel(UdpSocket socket) noexcept
    : socket_(std::move(socket)) {}

std::expected<std::size_t, UdpError>
UdpChannel::send(
    std::span<const std::byte> data,
    const Endpoint& destination
) {
    return socket_.send_to(data, destination);
}

void UdpChannel::set_receive_handler(ReceiveHandler handler) {
    receive_handler_ = std::move(handler);
}

void UdpChannel::set_error_handler(ErrorHandler handler) {
    error_handler_ = std::move(handler);
}

bool UdpChannel::is_open() const noexcept {
    return socket_.is_open();
}

std::expected<void, UdpError>
UdpChannel::poll() {
    while (true) {
        auto received = socket_.receive_many(
            receive_buffer_,
            max_datagram_size,
            receive_results_
        );

        if (!received) {
            if (received.error() == UdpError::WouldBlock) {
                return {};
            }

            if (error_handler_) {
                error_handler_(received.error());
            }

            return std::unexpected(received.error());
        }

        for (std::size_t i = 0; i < *received; ++i) {
            const auto& result = receive_results_[i];

            if (result.truncated) {
                if (error_handler_) {
                    error_handler_(UdpError::DatagramTruncated);
                }

                continue;
            }

            if (!result.sender) {
                if (error_handler_) {
                    error_handler_(UdpError::SystemError);
                }
                continue;
            }

            if (receive_handler_) {
                receive_handler_(
                    std::span<const std::byte>(
                        receive_buffer_.data() + i * max_datagram_size,
                        result.bytes_received
                    ),
                    *result.sender
                );
            }
        }

        if (*received == 0) {
            return {};
        }
    }
}

}