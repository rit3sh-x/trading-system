#include <cstddef>
#include <pthread.h>
#include <sched.h>

#include <trading/util/thread.hpp>

namespace trading::util {

bool pin_current_thread_to_core(int core_id) noexcept {
    if (core_id < 0 || core_id >= CPU_SETSIZE) {
        return false;
    }

    const auto core = static_cast<std::size_t>(core_id);

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(core, &cpu_set);

    return ::pthread_setaffinity_np(
        ::pthread_self(),
        sizeof(cpu_set),
        &cpu_set
    ) == 0;
}

}
