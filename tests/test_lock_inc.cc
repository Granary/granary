
#include "granary/globals.h"
#include "granary/code_cache.h"

#define TEST_LOCK 1

namespace test {

#if TEST_LOCK
    static int local_lock_inc(void) throw() {
        static volatile int i(0);
        ASM(
            "lock;"
            "incl %0;"
        : "=m"(i)
        );
        return i;
    }


    /// Test that `lock inc` is encoded correctly and behaves correctly on a
    /// memory operand.
    ///
    /// This is a good user space test on Mac OS X, which tends to lay the
    /// address space out differently and in such a way that `i` in
    /// `local_lock_inc` is > 4GB away from the code cache, which means extra
    /// mangling is done to ensure proper encoding.
    static void test_local_lock_inc(void) {
        granary::app_pc func((granary::app_pc) local_lock_inc);
        granary::basic_block bb_func(granary::code_cache<>::find(func));
        granary_break_on_bb(&bb_func);
        ASSERT(1 == bb_func.call<int>());
        ASSERT(2 == bb_func.call<int>());
        ASSERT(3 == bb_func.call<int>());
    }


    ADD_TEST(test_local_lock_inc,
        "Test that `lock inc` is encoded and behaves correctly.")

#endif
}
