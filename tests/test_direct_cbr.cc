
#include "granary/globals.h"
#include "granary/instruction.h"
#include "granary/basic_block.h"
#include "granary/state.h"

#define FOR_EACH_CBR(macro, ...) \
    macro(jo, ##__VA_ARGS__) \
    macro(jno, ##__VA_ARGS__) \
    macro(jb, ##__VA_ARGS__) \
    macro(jnb, ##__VA_ARGS__) \
    macro(jz, ##__VA_ARGS__) \
    macro(jnz, ##__VA_ARGS__) \
    macro(jbe, ##__VA_ARGS__) \
    macro(jnbe, ##__VA_ARGS__) \
    macro(js, ##__VA_ARGS__) \
    macro(jns, ##__VA_ARGS__) \
    macro(jp, ##__VA_ARGS__) \
    macro(jnp, ##__VA_ARGS__) \
    macro(jl, ##__VA_ARGS__) \
    macro(jnl, ##__VA_ARGS__) \
    macro(jle, ##__VA_ARGS__) \
    macro(jnle, ##__VA_ARGS__)


/// Make a short and "far" test for a conditional branch opcode. The
/// the target of the short jumps needs to be outside of the basic
/// block, which will stop at ret (otherwise it would be optimized into
/// and in-block jump).
#define MAKE_CBR_TEST_FUNC(opcode) \
    static void direct_cti_ ## opcode ## _short(void) throw() { \
        ASM( \
            #opcode " 1f;" \
            "mov $0, %rax;" \
            "ret;" \
        "1:  mov $1, %rax;" \
            "ret;" \
        ); \
    } \
    static void direct_cti_ ## opcode ## _long(void) throw() { \
        ASM( \
            #opcode " _start;" \
            "ret;" \
        ); \
    }

#define RUN_CBR_TEST_FUNC(opcode, code) \
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
    
    TEST(FOR_EACH_CBR(MAKE_CBR_TEST_FUNC))

    TEST(STATIC_INITIALIZE({
        granary::cpu_state_handle cpu;
        granary::thread_state_handle thread;
        FOR_EACH_CBR(RUN_CBR_TEST_FUNC, {
            break_on_cbr(&bb_short);
            break_on_cbr(&bb_long);

            granary::unsafe_cast<void (*)(void)>(bb_short.cache_pc_start)();
        })
    }))
}
