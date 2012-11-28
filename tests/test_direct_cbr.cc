
#include "granary/globals.h"
#include "granary/instruction.h"
#include "granary/basic_block.h"
#include "granary/state.h"

#include "granary/x86/asm_defines.asm"

/// For each jump type, expand some macro with enough info to generate test
/// code.
///
/// Note: not all possible condition codes need to be set here (especially for
///       things like jle where one of two conditions can be met); only one
///       satisfying condition needs to be met.
#define FOR_EACH_CBR(macro, ...) \
    macro(jo, OF, ~0, ##__VA_ARGS__) \
    macro(jno, 0, ~OF, ##__VA_ARGS__) \
    macro(jb, CF, ~0, ##__VA_ARGS__) \
    macro(jnb, 0, ~CF, ##__VA_ARGS__) \
    macro(jz, ZF, ~0, ##__VA_ARGS__) \
    macro(jnz, 0, ~ZF, ##__VA_ARGS__) \
    macro(jbe, (CF | ZF), ~0, ##__VA_ARGS__) \
    macro(jnbe, 0, ~(CF | ZF), ##__VA_ARGS__) \
    macro(js, SF, ~0, ##__VA_ARGS__) \
    macro(jns, 0, ~SF, ##__VA_ARGS__) \
    macro(jp, PF, ~0, ##__VA_ARGS__) \
    macro(jnp, 0, ~PF, ##__VA_ARGS__) \
    macro(jl, SF, ~0, ##__VA_ARGS__) \
    macro(jnl, 0, ~SF, ##__VA_ARGS__) \
    macro(jle, (ZF | SF), ~0, ##__VA_ARGS__) \
    macro(jnle, 0, ~(ZF | SF), ##__VA_ARGS__)


/// Make a short and "far" test for a conditional branch opcode. The
/// the target of the short jumps needs to be outside of the basic
/// block, which will stop at ret (otherwise it would be optimized into
/// and in-block jump).
///
/// Note: these tests, unfortunately, depend on frame pointers being present.
#define MAKE_CBR_TEST_FUNC(opcode, or_flag, and_flag) \
    static void direct_cti_ ## opcode ## _short(void) throw() { \
        ASM( \
            "pushf;" \
            "movq $%c0, %%rax;" \
            "or %%rax, (%%rsp);" \
            "movq $%c1, %%rax;" \
            "and %%rax, (%%rsp);" \
            "popf;" \
            #opcode " 1f;" \
            "mov $0, %%rax;" \
            "ret;" \
        "1:  mov $1, %%rax;" \
            "mov %%rbp, %%rsp;" \
            "pop %%rbp;" \
            "ret;" \
            : \
            : "i"(or_flag), "i"(and_flag) \
        ); \
    } \
    static void direct_cti_ ## opcode ## _long(void) throw() { \
        ASM( \
            "pushf;" \
            "movq $%c0, %%rax;" \
            "or %%rax, (%%rsp);" \
            "movq $%c1, %%rax;" \
            "and %%rax, (%%rsp);" \
            "popf;" \
            "mov %%rbp, %%rsp;" \
            "pop %%rbp;" \
            #opcode " " TO_STRING(SYMBOL(granary_test_return_true)) ";" \
            "mov $0, %%rax;" \
            "mov %%rbp, %%rsp;" \
            "pop %%rbp;" \
            "ret;" \
            : \
            : "i"(or_flag), "i"(and_flag) \
        ); \
    }

#define RUN_CBR_TEST_FUNC(opcode, or_flag, and_flag, code) \
    { \
        granary::app_pc short_cbr((granary::app_pc) direct_cti_ ## opcode ## _short); \
        granary::basic_block bb_short(granary::basic_block::translate( \
            cpu, thread, &short_cbr)); \
        \
        granary::app_pc long_cbr((granary::app_pc) direct_cti_ ## opcode ## _long); \
        granary::basic_block bb_long(granary::basic_block::translate( \
            cpu, thread, &long_cbr)); \
        \
        code \
    }

static void break_on_cbr(granary::basic_block *bb) {
    (void) bb;
}

namespace test {

    enum {
        CF      = (1 << 0), // carry
        PF      = (1 << 2), // parity
        AF      = (1 << 4), // adjust
        ZF      = (1 << 6), // zero
        SF      = (1 << 7), // sign
        DF      = (1 << 10), // direction
        OF      = (1 << 11) // overflow
    };
    
    IF_TEST(FOR_EACH_CBR(MAKE_CBR_TEST_FUNC))

    IF_TEST(static void direct_cbrs_patched_correctly(void) {
        granary::cpu_state_handle cpu;
        granary::thread_state_handle thread;
        FOR_EACH_CBR(RUN_CBR_TEST_FUNC, {
            break_on_cbr(&bb_short);
            break_on_cbr(&bb_long);

            ASSERT(bb_short.call<bool>());
            ASSERT(bb_long.call<bool>());
        })
    })

    ADD_TEST(direct_cbrs_patched_correctly,
        "Test that targets of direct conditional branches are correctly patched.")
}
