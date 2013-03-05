/*
 * main.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstdio>
#include <cstdlib>
#include "granary/globals.h"

#define ENABLE_SEGFAULT_HANDLER 1

#if ENABLE_SEGFAULT_HANDLER
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

void segfault_handler(int) {
    printf("Run `gdb attach %d`\n", getpid());
    for(;;) { /* loop until we manually attach gdb */ }
}

#endif

#include <chrono>
struct clock
{
    typedef unsigned long long                 rep;
    typedef std::ratio<1, 2260000000>          period; // My machine is 2.26 GHz
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<clock>     time_point;
    static const bool is_steady =              true;

    static time_point now() throw()
    {
        unsigned lo, hi;
        asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
        return time_point(duration(static_cast<rep>(hi) << 32 | lo));
    }
};

enum {
    ITERATIONS = 10000000
};

__attribute__((noinline, optimize("O0")))
void indirect_call_target(void) throw() {
    ASM("");
}

void (*indirect_func)(void) = indirect_call_target;

__attribute__((noinline, optimize("O0")))
void make_indirect_call(void) throw() {
    for(int i = 0; i < ITERATIONS; ++i) {
        indirect_func();
    }
}

__attribute__((noinline, optimize("O0")))
void make_direct_call(void) throw() {
    for(int i = 0; i < ITERATIONS; ++i) {
        indirect_call_target();
    }
}

#include "granary/test.h"
int main(int argc, const char **argv) throw() {
#if ENABLE_SEGFAULT_HANDLER
    signal(SIGSEGV, segfault_handler);
#endif
    (void) argc;
    (void) argv;

    granary::init();

#if CONFIG_RUN_TEST_CASES
    granary::run_tests();
#endif

    granary::basic_block in_bb(granary::code_cache::find(
        (granary::app_pc) make_indirect_call,
        granary::policy_for<granary::test_policy>()));

    granary::basic_block dir_bb(granary::code_cache::find(
        (granary::app_pc) make_direct_call,
        granary::policy_for<granary::test_policy>()));

    // warm up the code cache
    in_bb.call<void>();
    dir_bb.call<void>();

    printf("\n\n\n\n");

    clock::time_point in_start = clock::now();
    in_bb.call<void>();
    clock::time_point in_end = clock::now();
    make_indirect_call();
    clock::time_point in_native_end = clock::now();
    dir_bb.call<void>();
    clock::time_point dir_end = clock::now();
    make_direct_call();
    clock::time_point dir_native_end = clock::now();

    typedef std::chrono::duration<double, typename clock::period> Cycle;
    auto ticks_per_iter = Cycle(in_end-in_start)/ITERATIONS;

    printf("%lf clock ticks per instrumented iteration (indirect)\n",
        ticks_per_iter.count());

    ticks_per_iter = Cycle(in_native_end - in_end)/ITERATIONS;

    printf("%lf clock ticks per native iteration (indirect)\n",
        ticks_per_iter.count());

    ticks_per_iter = Cycle(dir_end - in_native_end)/ITERATIONS;
    printf("%lf clock ticks per instrumented iteration (direct)\n",
        ticks_per_iter.count());

    ticks_per_iter = Cycle(dir_native_end - dir_end)/ITERATIONS;

    printf("%lf clock ticks per native iteration (direct)\n",
        ticks_per_iter.count());

    IF_PERF( granary::perf::report(); )

    return 0;
}

