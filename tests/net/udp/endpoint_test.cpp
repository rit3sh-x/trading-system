#include <catch2/catch_test_macros.hpp>

#include <netinet/in.h>

#include <trading/net/udp/endpoint.hpp>

using namespace trading::net::udp;

TEST_CASE("Endpoint creates valid IPv4 address", "[endpoint]") {
    const auto result = Endpoint::create("127.0.0.1", 8080);

    REQUIRE(result.has_value());
    REQUIRE(result->address() == "127.0.0.1");
    REQUIRE(result->port() == 8080);
}

TEST_CASE("Endpoint allows port zero", "[endpoint]") {
    const auto result = Endpoint::create("127.0.0.1", 0);

    REQUIRE(result.has_value());
    REQUIRE(result->address() == "127.0.0.1");
    REQUIRE(result->port() == 0);
}

TEST_CASE("Endpoint rejects invalid IPv4 address", "[endpoint]") {
    const auto result = Endpoint::create("999.999.999.999", 8080);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == EndpointError::InvalidAddress);
}

TEST_CASE("Endpoint rejects non-IP address", "[endpoint]") {
    const auto result = Endpoint::create("not-an-ip", 8080);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == EndpointError::InvalidAddress);
}

TEST_CASE("Endpoint converts IPv4 to sockaddr_storage", "[endpoint]") {
    const auto result = Endpoint::create("127.0.0.1", 8080);

    REQUIRE(result.has_value());

    const auto storage = result->to_sockaddr();

    REQUIRE(storage.ss_family == AF_INET);

    const auto* address =
        reinterpret_cast<const sockaddr_in*>(&storage);

    REQUIRE(ntohs(address->sin_port) == 8080);
}

TEST_CASE("Endpoint converts sockaddr_storage back to IPv4", "[endpoint]") {
    const auto original = Endpoint::create("127.0.0.1", 8080);

    REQUIRE(original.has_value());

    const auto storage = original->to_sockaddr();
    const auto result = Endpoint::from_sockaddr(storage);

    REQUIRE(result.has_value());
    REQUIRE(result->address() == "127.0.0.1");
    REQUIRE(result->port() == 8080);
}

TEST_CASE("Endpoint rejects unsupported sockaddr family", "[endpoint]") {
    sockaddr_storage storage{};
    storage.ss_family = AF_UNSPEC;

    const auto result = Endpoint::from_sockaddr(storage);

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == EndpointError::InvalidSockaddr);
}

TEST_CASE("Equal endpoints compare equal", "[endpoint]") {
    const auto first = Endpoint::create("127.0.0.1", 8080);

    const auto second = Endpoint::create("127.0.0.1", 8080);

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());

    REQUIRE(*first == *second);
}

TEST_CASE("Different ports make endpoints unequal", "[endpoint]") {
    const auto first = Endpoint::create("127.0.0.1", 8080);

    const auto second = Endpoint::create("127.0.0.1", 8081);

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());

    REQUIRE(*first != *second);
}