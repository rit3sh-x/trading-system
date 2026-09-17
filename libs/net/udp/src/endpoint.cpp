#include <trading/net/udp/endpoint.hpp>

namespace trading::net::udp {

Endpoint::Endpoint(
    std::string_view address,
    std::uint16_t port
)
    : address_(address),
      port_(port) {}

const std::string& Endpoint::address() const noexcept {
    return address_;
}

std::uint16_t Endpoint::port() const noexcept {
    return port_;
}

std::expected<Endpoint, EndpointError>
Endpoint::create(
    std::string_view address,
    std::uint16_t port
) {
    in_addr ipv4{};

    if (::inet_pton(
            AF_INET,
            std::string(address).c_str(),
            &ipv4
        ) != 1) {
        return std::unexpected(EndpointError::InvalidAddress);
    }

    return Endpoint(address, port);
}

sockaddr_storage Endpoint::to_sockaddr() const noexcept {
    sockaddr_storage storage{};

    auto* address =
        reinterpret_cast<sockaddr_in*>(&storage);

    address->sin_family = AF_INET;
    address->sin_port = htons(port_);

    ::inet_pton(
        AF_INET,
        address_.c_str(),
        &address->sin_addr
    );

    return storage;
}

std::expected<Endpoint, EndpointError>
Endpoint::from_sockaddr(
    const sockaddr_storage& storage
) {
    if (storage.ss_family != AF_INET) {
        return std::unexpected(EndpointError::InvalidSockaddr);
    }

    const auto* address =
        reinterpret_cast<const sockaddr_in*>(&storage);

    char address_buffer[INET_ADDRSTRLEN]{};

    if (::inet_ntop(
            AF_INET,
            &address->sin_addr,
            address_buffer,
            sizeof(address_buffer)
        ) == nullptr) {
        return std::unexpected(EndpointError::InvalidSockaddr);
    }

    return Endpoint(
        address_buffer,
        ntohs(address->sin_port)
    );
}

}