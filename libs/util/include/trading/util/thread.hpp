#pragma once

namespace trading::util {

// Pin the calling thread to a single CPU core.
//
// Removing the scheduler's freedom to migrate the thread keeps its L1/L2
// cache and branch-predictor state warm, which matters for the hot polling
// loops in this system. Call it once from the thread you want to isolate,
// after that thread has started running.
[[nodiscard]]
bool pin_current_thread_to_core(int core_id) noexcept;

}
