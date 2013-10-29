/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_direct_call.cc
 *
 *  Created on: 2012-11-30
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#define TEST_CALL 1

namespace test {

#if TEST_CALL

    static int direct_call_short(void) {
        register int64_t ret asm("rcx") = 0;
        ASM(
            "mov $0, %%rax;"
            "call 1f;"
            "movq %%rax, %0;"
            "jmp 2f;"
        "1:  mov $1, %%rax;"
            "ret;"
        "2:"
            : "=r"(ret)
        );
        return ret;
    }

    static int direct_call_far(void) {
        register int64_t ret asm("rcx") = 0;
        ASM(
            "mov $0, %%rax;"
            "call " TO_STRING(SYMBOL(granary_test_return_true)) ";"
            "movq %%rax, %0;"
            : "=r"(ret)
        );
        return ret;
    }


    /// Test jcxz/jecxz/jrcxz; Note: there is no far version of jrxcz.
    static void direct_call_patched_correctly(void) {
        granary::app_pc call_direct_short((granary::app_pc) direct_call_short);
        granary::basic_block bb_call_direct_short(granary::code_cache::find(
            call_direct_short, granary::TEST_POLICY));

        granary::app_pc call_direct_far((granary::app_pc) direct_call_far);
        granary::basic_block bb_call_direct_far(granary::code_cache::find(
            call_direct_far, granary::TEST_POLICY));

        ASSERT(bb_call_direct_short.call<bool>());
        ASSERT(bb_call_direct_far.call<bool>());
    }


    ADD_TEST(direct_call_patched_correctly,
        "Test that targets of direct call are correctly patched.")
#endif
}

#endif
