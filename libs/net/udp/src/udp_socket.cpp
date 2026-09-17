#include <algorithm>
#include <array>
#include <arpa/inet.h>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include <trading/net/udp/udp_socket.hpp>

namespace trading::net::udp {

namespace {

constexpr std::size_t max_batch_capacity = 64;

}

UdpSocket::~UdpSocket() {
    if (socket_ != -1) {
        ::close(socket_);
    }
}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
    : socket_(other.socket_),
      statistics_(std::move(other.statistics_)) {
        other.socket_ = -1;
    }

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        if (socket_ != -1) {
            ::close(socket_);
        }

        socket_ = other.socket_;
        statistics_ = std::move(other.statistics_);

        other.socket_ = -1;
    }

    return *this;
}

std::expected<void, UdpError>
UdpSocket::close() noexcept {
    if (!is_open()) {
        return {};
    }

    if (::close(socket_) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    socket_ = -1;
    return {};
}

bool UdpSocket::is_open() const noexcept {
    return socket_ >= 0;
}

std::expected<void, UdpError>
UdpSocket::bind(const Endpoint& endpoint) {
    if (is_open()) {
        return std::unexpected(UdpError::AlreadyBound);
    }

    socket_ = ::socket(
        AF_INET,
        SOCK_DGRAM,
        IPPROTO_UDP
    );

    if (socket_ == -1) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    const auto address = endpoint.to_sockaddr();

    if (::bind(
            socket_,
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(sockaddr_in)
        ) == -1) {

        const int error = errno;

        ::close(socket_);
        socket_ = -1;

        if (error == EADDRINUSE) {
            return std::unexpected(UdpError::AddressAlreadyInUse);
        }

        return std::unexpected(UdpError::BindFailed);
    }

    return {};
}

std::expected<std::size_t, UdpError>
UdpSocket::send_to(
    std::span<const std::byte> data,
    const Endpoint& destination
) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    const auto address = destination.to_sockaddr();

    const auto result = ::sendto(
        socket_,
        data.data(),
        data.size(),
        0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(sockaddr_in)
    );

    if (result == -1) {
        const int error = errno;

        if (error == EAGAIN || error == EWOULDBLOCK) {
            statistics_.record_would_block();

            return std::unexpected(UdpError::WouldBlock);
        }

        statistics_.record_send_error();

        if (error == EMSGSIZE) {
            return std::unexpected(UdpError::MessageTooLarge);
        }

        return std::unexpected(UdpError::SendFailed);
    }

    const auto bytes = static_cast<std::size_t>(result);

    statistics_.record_datagram_sent(bytes);

    return static_cast<std::size_t>(result);
}

std::expected<ReceiveResult, UdpError>
UdpSocket::receive_from(std::span<std::byte> buffer) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    sockaddr_storage sender{};
    iovec iovec{};

    iovec.iov_base = buffer.data();
    iovec.iov_len = buffer.size();

    msghdr message{};

    message.msg_name = &sender;
    message.msg_namelen = sizeof(sender);
    message.msg_iov = &iovec;
    message.msg_iovlen = 1;

    const auto result = ::recvmsg(
        socket_,
        &message,
        MSG_TRUNC
    );

    if (result == -1) {
        const int error = errno;

        if (error == EAGAIN || error == EWOULDBLOCK) {
            statistics_.record_would_block();

            return std::unexpected(UdpError::WouldBlock);
        }

        statistics_.record_receive_error();

        return std::unexpected(UdpError::ReceiveFailed);
    }

    const auto endpoint = Endpoint::from_sockaddr(sender);

    if (!endpoint) {
        statistics_.record_receive_error();

        return std::unexpected(UdpError::SystemError);
    }

    const bool truncated = (message.msg_flags & MSG_TRUNC) != 0;

    const auto bytes = static_cast<std::size_t>(result);

    statistics_.record_datagram_received(
        bytes,
        truncated
    );

    return ReceiveResult{
        .bytes_received = bytes,
        .sender = *endpoint,
        .truncated = truncated
    };
}

std::expected<std::size_t, UdpError>
UdpSocket::receive_many(
    std::span<std::byte> buffer,
    std::size_t datagram_capacity,
    std::span<ReceiveResult> results
) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    const std::size_t requested =
        std::min(results.size(), max_batch_capacity);

    if (requested == 0 || datagram_capacity == 0) {
        return static_cast<std::size_t>(0);
    }

    if (buffer.size() < requested * datagram_capacity) {
        return std::unexpected(UdpError::SystemError);
    }

    std::array<mmsghdr, max_batch_capacity> headers;
    std::array<iovec, max_batch_capacity> iovecs;
    std::array<sockaddr_storage, max_batch_capacity> senders;

    for (std::size_t i = 0; i < requested; ++i) {
        iovecs[i].iov_base = buffer.data() + i * datagram_capacity;
        iovecs[i].iov_len = datagram_capacity;

        headers[i].msg_hdr.msg_name = &senders[i];
        headers[i].msg_hdr.msg_namelen = sizeof(sockaddr_storage);
        headers[i].msg_hdr.msg_iov = &iovecs[i];
        headers[i].msg_hdr.msg_iovlen = 1;
        headers[i].msg_hdr.msg_control = nullptr;
        headers[i].msg_hdr.msg_controllen = 0;
        headers[i].msg_hdr.msg_flags = 0;
        headers[i].msg_len = 0;
    }

    const int received = ::recvmmsg(
        socket_,
        headers.data(),
        static_cast<unsigned int>(requested),
        MSG_WAITFORONE | MSG_TRUNC,
        nullptr
    );

    if (received == -1) {
        const int error = errno;

        if (error == EAGAIN || error == EWOULDBLOCK) {
            statistics_.record_would_block();
            return std::unexpected(UdpError::WouldBlock);
        }

        statistics_.record_receive_error();

        return std::unexpected(UdpError::ReceiveFailed);
    }

    const auto count = static_cast<std::size_t>(received);

    statistics_.record_batch(count);

    for (std::size_t i = 0; i < count; ++i) {
        const bool truncated =
            (headers[i].msg_hdr.msg_flags & MSG_TRUNC) != 0
            || headers[i].msg_len > datagram_capacity;

        const auto endpoint = Endpoint::from_sockaddr(senders[i]);

        if (!endpoint) {
            statistics_.record_receive_error();

            return std::unexpected(UdpError::SystemError);
        }

        const auto bytes = static_cast<std::size_t>(headers[i].msg_len);

        statistics_.record_datagram_received(
            bytes,
            truncated
        );

        results[i] = ReceiveResult{
            .bytes_received = static_cast<std::size_t>(headers[i].msg_len),
            .sender = *endpoint,
            .truncated = truncated
        };
    }

    return count;
}

std::expected<void, UdpError>
UdpSocket::set_non_blocking(bool enabled) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    const int flags = ::fcntl(
        socket_,
        F_GETFL,
        0
    );

    if (flags == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    const int new_flags = enabled
        ? flags | O_NONBLOCK
        : flags & ~O_NONBLOCK;

    if (::fcntl(
            socket_,
            F_SETFL,
            new_flags
        ) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    return {};
}

std::expected<void, UdpError>
UdpSocket::set_receive_buffer(std::size_t size) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    if (size > static_cast<std::size_t>(INT_MAX)) {
        return std::unexpected(UdpError::SystemError);
    }

    const int value = static_cast<int>(size);

    if (::setsockopt(
            socket_,
            SOL_SOCKET,
            SO_RCVBUF,
            &value,
            sizeof(value)
        ) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    return {};
}

std::expected<void, UdpError>
UdpSocket::set_send_buffer(std::size_t size) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    if (size > static_cast<std::size_t>(INT_MAX)) {
        return std::unexpected(UdpError::SystemError);
    }

    const int value = static_cast<int>(size);

    if (::setsockopt(
            socket_,
            SOL_SOCKET,
            SO_SNDBUF,
            &value,
            sizeof(value)
        ) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    return {};
}

std::expected<Endpoint, UdpError>
UdpSocket::local_endpoint() const {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    sockaddr_storage address{};
    socklen_t length = sizeof(address);

    if (::getsockname(
            socket_,
            reinterpret_cast<sockaddr*>(&address),
            &length
        ) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    const auto endpoint = Endpoint::from_sockaddr(address);

    if (!endpoint) {
        return std::unexpected(UdpError::SystemError);
    }

    return *endpoint;
}

std::expected<void, UdpError>
UdpSocket::join_multicast_group(const Endpoint& multicast_endpoint) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    ip_mreq request{};

    if (::inet_pton(
            AF_INET,
            multicast_endpoint.address().c_str(),
            &request.imr_multiaddr
        ) != 1) {
        return std::unexpected(UdpError::InvalidAddress);
    }

    request.imr_interface.s_addr = htonl(INADDR_ANY);

    if (::setsockopt(
            socket_,
            IPPROTO_IP,
            IP_ADD_MEMBERSHIP,
            &request,
            sizeof(request)
        ) == -1) {
        return std::unexpected(UdpError::MulticastJoinFailed);
    }

    return {};
}

std::expected<void, UdpError>
UdpSocket::leave_multicast_group(const Endpoint& multicast_endpoint) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    ip_mreq request{};

    if (::inet_pton(
            AF_INET,
            multicast_endpoint.address().c_str(),
            &request.imr_multiaddr
        ) != 1) {
        return std::unexpected(UdpError::InvalidAddress);
    }

    request.imr_interface.s_addr = htonl(INADDR_ANY);

    if (::setsockopt(
            socket_,
            IPPROTO_IP,
            IP_DROP_MEMBERSHIP,
            &request,
            sizeof(request)
        ) == -1) {
        return std::unexpected(UdpError::MulticastLeaveFailed);
    }

    return {};
}

std::expected<void, UdpError>
UdpSocket::set_multicast_interface(const Endpoint& interface) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    const auto address = interface.to_sockaddr();

    const auto* addr = reinterpret_cast<const sockaddr_in*>(&address);

    if (::setsockopt(
            socket_,
            IPPROTO_IP,
            IP_MULTICAST_IF,
            &addr->sin_addr,
            sizeof(addr->sin_addr)
        ) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    return {};
}

std::expected<void, UdpError>
UdpSocket::set_multicast_loopback(bool enabled) {
    if (!is_open()) {
        return std::unexpected(UdpError::SocketCreationFailed);
    }

    const unsigned char value = enabled ? 1 : 0;

    if (::setsockopt(
            socket_,
            IPPROTO_IP,
            IP_MULTICAST_LOOP,
            &value,
            sizeof(value)
        ) == -1) {
        return std::unexpected(UdpError::SystemError);
    }

    return {};
}

UdpStatistics
UdpSocket::statistics() const noexcept {
    return statistics_;
}

}
