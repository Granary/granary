/*
 * test_direct_detach_call.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */


#include "granary/globals.h"
#include "granary/code_cache.h"
#include "granary/x86/asm_defines.asm"

#define TEST_DETACH_CALL 1

namespace test {

#if TEST_DETACH_CALL

    static int direct_detach_call(void) {
        ASM(
            "mov $0, %rax;"
            "call 1f;"
            "mov %rbp, %rsp;"
            "pop %rbp;"
            "ret;"
        "1:  mov $1, %rax;"
            "ret;"
        );
        return 0; // not reached
    }


    /// Test jcxz/jecxz/jrcxz; Note: there is no far version of jrxcz.
    static void direct_call_patched_correctly(void) {
        granary::app_pc call_direct_short((granary::app_pc) direct_call_short);
        granary::basic_block bb_call_direct_short(granary::code_cache<>::find(call_direct_short));
        granary::app_pc call_direct_far((granary::app_pc) direct_call_far);
        granary::basic_block bb_call_direct_far(granary::code_cache<>::find(call_direct_far));

        ASSERT(bb_call_direct_short.call<bool>());
        ASSERT(bb_call_direct_far.call<bool>());
    }


    ADD_TEST(direct_call_patched_correctly,
        "Test that targets of direct call are correctly patched.")
#endif
}


