#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <sys/socket.h>

namespace trading::net::udp {

// Errors that can occur while working with endpoints.
//
// Use a one-byte underlying type to keep the enum's memory footprint small.
enum class EndpointError : std::uint8_t {
    InvalidAddress,
    InvalidSockaddr
};

class Endpoint {
public:
    // Creates an endpoint from an IPv4 address and port.
    //
    // Returns InvalidAddress if the address is not a valid IPv4 address.
    [[nodiscard]]
    static std::expected<Endpoint, EndpointError>
    create(
        std::string_view address,
        std::uint16_t port
    );

    // Creates an endpoint from a sockaddr_storage structure.
    //
    // Returns InvalidSockaddr if the address is not a supported
    // or valid sockaddr structure.
    [[nodiscard]]
    static std::expected<Endpoint, EndpointError>
    from_sockaddr(const sockaddr_storage& address);

    // Returns the IPv4 address as a string.
    [[nodiscard]]
    const std::string& address() const noexcept;

    // Returns the endpoint's port.
    [[nodiscard]]
    std::uint16_t port() const noexcept;

    // Converts the endpoint to a sockaddr_storage suitable for socket APIs.
    [[nodiscard]]
    sockaddr_storage to_sockaddr() const noexcept;

    bool operator==(const Endpoint&) const = default;

private:
    Endpoint(
        std::string_view address,
        std::uint16_t port
    );

    std::string address_;
    std::uint16_t port_;
};

}