#pragma once

#include <cstddef>
#include <cstdint>

namespace trading::net::udp {

class UdpStatistics {
public:
    void record_datagram_received(
        std::size_t bytes,
        bool truncated
    ) noexcept;

    void record_datagram_sent(
        std::size_t bytes
    ) noexcept;

    void record_send_error() noexcept;
    void record_receive_error() noexcept;
    void record_would_block() noexcept;

    void record_batch(
        std::size_t size
    ) noexcept;

    void record_kernel_drops(
        std::uint64_t count
    ) noexcept;

    void reset() noexcept;

    [[nodiscard]]
    std::uint64_t datagrams_received() const noexcept {
        return datagrams_received_;
    }

    [[nodiscard]]
    std::uint64_t bytes_received() const noexcept {
        return bytes_received_;
    }

    [[nodiscard]]
    std::uint64_t datagrams_sent() const noexcept {
        return datagrams_sent_;
    }

    [[nodiscard]]
    std::uint64_t bytes_sent() const noexcept {
        return bytes_sent_;
    }

    [[nodiscard]]
    std::uint64_t datagrams_truncated() const noexcept {
        return datagrams_truncated_;
    }

    [[nodiscard]]
    std::uint64_t send_errors() const noexcept {
        return send_errors_;
    }

    [[nodiscard]]
    std::uint64_t receive_errors() const noexcept {
        return receive_errors_;
    }

    [[nodiscard]]
    std::uint64_t would_block() const noexcept {
        return would_block_;
    }

    [[nodiscard]]
    std::uint64_t batches_received() const noexcept {
        return batches_received_;
    }

    [[nodiscard]]
    std::uint64_t max_batch_size() const noexcept {
        return max_batch_size_;
    }

    [[nodiscard]]
    std::uint64_t kernel_drops() const noexcept {
        return kernel_drops_;
    }

private:
    std::uint64_t datagrams_received_{0};
    std::uint64_t bytes_received_{0};

    std::uint64_t datagrams_sent_{0};
    std::uint64_t bytes_sent_{0};

    std::uint64_t datagrams_truncated_{0};

    std::uint64_t send_errors_{0};
    std::uint64_t receive_errors_{0};
    std::uint64_t would_block_{0};

    std::uint64_t batches_received_{0};
    std::uint64_t max_batch_size_{0};

    std::uint64_t kernel_drops_{0};
};

}