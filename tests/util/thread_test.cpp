#include <sched.h>

#include <catch2/catch_test_macros.hpp>

#include <trading/util/thread.hpp>

using trading::util::pin_current_thread_to_core;

TEST_CASE("pin_current_thread_to_core pins to a valid core", "[util][thread]") {
    const int core = sched_getcpu();
    REQUIRE(core >= 0);

    REQUIRE(pin_current_thread_to_core(core));

    sched_yield();
    REQUIRE(sched_getcpu() == core);
}

TEST_CASE("pin_current_thread_to_core rejects invalid cores", "[util][thread]") {
    REQUIRE_FALSE(pin_current_thread_to_core(-1));
    REQUIRE_FALSE(pin_current_thread_to_core(1'000'000));
}
