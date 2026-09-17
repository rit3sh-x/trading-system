#include <trading/net/udp/statistics.hpp>

namespace trading::net::udp {

void UdpStatistics::record_datagram_received(
    std::size_t bytes,
    bool truncated
) noexcept {
    ++datagrams_received_;
    bytes_received_ += bytes;

    if (truncated) {
        ++datagrams_truncated_;
    }
}

void UdpStatistics::record_datagram_sent(
    std::size_t bytes
) noexcept {
    ++datagrams_sent_;
    bytes_sent_ += bytes;
}

void UdpStatistics::record_send_error() noexcept {
    ++send_errors_;
}

void UdpStatistics::record_receive_error() noexcept {
    ++receive_errors_;
}

void UdpStatistics::record_would_block() noexcept {
    ++would_block_;
}

void UdpStatistics::record_batch(
    std::size_t size
) noexcept {
    ++batches_received_;

    if (size > max_batch_size_) {
        max_batch_size_ = size;
    }
}

void UdpStatistics::record_kernel_drops(
    std::uint64_t count
) noexcept {
    kernel_drops_ += count;
}

void UdpStatistics::reset() noexcept {
    datagrams_received_ = 0;
    bytes_received_ = 0;

    datagrams_sent_ = 0;
    bytes_sent_ = 0;

    datagrams_truncated_ = 0;

    send_errors_ = 0;
    receive_errors_ = 0;
    would_block_ = 0;

    batches_received_ = 0;
    max_batch_size_ = 0;

    kernel_drops_ = 0;
}

}