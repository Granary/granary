/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * perf.cc
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/perf.h"

#if CONFIG_ENABLE_PERF_COUNTS

#include "granary/instruction.h"
#include "granary/basic_block.h"
#include "granary/state.h"
#include "granary/atomic.h"
#include "granary/printf.h"
#include "granary/detach.h"
#include "granary/ibl.h"

extern "C" {
    int sprintf(char *, const char *, ...);
}

namespace granary {


    /// Performance counters for tracking decoded instructions.
    static std::atomic<unsigned> NUM_DECODED_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_DECODED_BYTES(ATOMIC_VAR_INIT(0U));


    /// Performance counters for tracking encoded instructions.
    static std::atomic<unsigned> NUM_ENCODED_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_ENCODED_BYTES(ATOMIC_VAR_INIT(0U));


    /// Performance counter for tracking basic blocks and their instructions.
    static std::atomic<unsigned> NUM_BBS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_BB_INSTRUCTION_BYTES(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_BB_PATCH_BYTES(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_BB_STATE_BYTES(ATOMIC_VAR_INIT(0U));


    /// Performance counters for tracking different types of indirect CTIs.
    static std::atomic<unsigned> NUM_INDIRECT_JMPS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_INDIRECT_CALLS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_RETURNS(ATOMIC_VAR_INIT(0U));


    /// Performance counters for tracking IBL, and DBL instruction counts.
    static std::atomic<unsigned> NUM_IBL_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_IBL_ENTRY_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_IBL_EXIT_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_IBL_HTABLE_ENTRIES(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_IBL_MISSES(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_IBL_CONFLICTS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_DBL_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_DBL_STUB_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_DBL_PATCH_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_RBL_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));


    /// Performance counters for tracking instructions added in order to mangle
    /// memory references.
    static std::atomic<unsigned> NUM_MEM_REF_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));


    /// NOPs added to get specific alignments.
    static std::atomic<unsigned> NUM_ALIGN_NOP_INSTRUCTIONS(ATOMIC_VAR_INIT(0U));


    /// Tracking the number if code cache address lookups.
    static std::atomic<unsigned> NUM_ADDRESS_LOOKUPS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_ADDRESS_LOOKUP_HITS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_ADDRESS_LOOKUPS_CPU_HIT(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_ADDRESS_LOOKUPS_CPU_MISS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned> NUM_ADDRESS_LOOKUPS_CPU_MISPREDICT(ATOMIC_VAR_INIT(0U));


#if GRANARY_IN_KERNEL
    static std::atomic<unsigned long> NUM_INTERRUPTS(ATOMIC_VAR_INIT(0UL));
    static std::atomic<unsigned> NUM_RECURSIVE_INTERRUPTS(ATOMIC_VAR_INIT(0U));
    static std::atomic<unsigned long> NUM_DELAYED_INTERRUPTS(ATOMIC_VAR_INIT(0UL));
    static std::atomic<unsigned long> NUM_BAD_MODULE_EXECS(ATOMIC_VAR_INIT(0UL));
#endif


    void perf::visit_address_lookup(void) throw() {
        NUM_ADDRESS_LOOKUPS.fetch_add(1);
    }

    void perf::visit_address_lookup_cpu(bool hit) throw() {
        if(hit) {
            NUM_ADDRESS_LOOKUPS_CPU_HIT.fetch_add(1);
        } else {
            NUM_ADDRESS_LOOKUPS_CPU_MISS.fetch_add(1);
        }
    }


    void perf::visit_address_lookup_hit(void) throw() {
        NUM_ADDRESS_LOOKUP_HITS.fetch_add(1);
    }


    void perf::visit_decoded(const instruction in) throw() {
        if(in.is_valid()) {
            NUM_DECODED_INSTRUCTIONS.fetch_add(1);
            NUM_DECODED_BYTES.fetch_add(in.instr->length);
        }
    }


    void perf::visit_encoded(const instruction in) throw() {
        if(in.is_valid()) {
            NUM_ENCODED_INSTRUCTIONS.fetch_add(1);
            NUM_ENCODED_BYTES.fetch_add(in.instr->length);
        }
    }


    void perf::visit_encoded(const basic_block &bb) throw() {
        NUM_BBS.fetch_add(1);
        NUM_BB_INSTRUCTION_BYTES.fetch_add(
            bb.info->num_bytes - bb.info->num_patch_bytes);
        NUM_BB_PATCH_BYTES.fetch_add(bb.info->num_patch_bytes);
        NUM_BB_STATE_BYTES.fetch_add(sizeof(basic_block_state));
    }


    void perf::visit_mangle_indirect_jmp(void) throw() {
        NUM_INDIRECT_JMPS.fetch_add(1);
    }


    void perf::visit_mangle_indirect_call(void) throw() {
        NUM_INDIRECT_CALLS.fetch_add(1);
    }


    void perf::visit_mangle_return(void) throw() {
        NUM_RETURNS.fetch_add(1);
    }


    void perf::visit_ibl(const instruction_list &ls) throw() {
        NUM_IBL_INSTRUCTIONS.fetch_add(ls.length());
    }


    void perf::visit_ibl_stub(unsigned num_instructions) throw() {
        NUM_IBL_ENTRY_INSTRUCTIONS.fetch_add(num_instructions);
    }


    void perf::visit_ibl_exit(const instruction_list &ls) throw() {
        NUM_IBL_EXIT_INSTRUCTIONS.fetch_add(ls.length());
    }

    struct ibl_entry {
        app_pc target;
        IF_PROFILE_IBL( app_pc source; )
        IF_PROFILE_IBL( uint64_t *count; )
    };

    enum {
        NUM_IBL_PROFILE_ENTRIES = NUM_IBL_JUMP_TABLE_ENTRIES IF_PROFILE_IBL(* 4)
    };

    static ibl_entry IBL_TARGETS[NUM_IBL_PROFILE_ENTRIES] = {
        {nullptr _IF_PROFILE_IBL(nullptr, nullptr) }
    };

    static std::atomic<uint8_t> IB_USE_COUNT[
        NUM_IBL_JUMP_TABLE_ENTRIES
    ] = {ATOMIC_VAR_INIT(0)};


    void perf::visit_ibl_add_entry(
        app_pc pc
        _IF_PROFILE_IBL( app_pc source_addr )
        _IF_PROFILE_IBL( uint64_t *count )
    ) throw() {
        const unsigned i(NUM_IBL_HTABLE_ENTRIES.fetch_add(1));

        if(i < NUM_IBL_PROFILE_ENTRIES) {
            IBL_TARGETS[i].target = pc;
            IF_PROFILE_IBL( IBL_TARGETS[i].source = source_addr; )
            IF_PROFILE_IBL( IBL_TARGETS[i].count = count; )
        }
        IB_USE_COUNT[granary_ibl_hash(pc)].fetch_add(1);
    }


    void perf::visit_ibl_miss(app_pc) throw() {
        NUM_IBL_MISSES.fetch_add(1);
    }


    void perf::visit_ibl_conflict(app_pc) throw() {
        NUM_IBL_CONFLICTS.fetch_add(1);
    }


    void perf::visit_dbl(const instruction_list &ls) throw() {
        NUM_DBL_INSTRUCTIONS.fetch_add(ls.length());
    }


    void perf::visit_dbl_patch(const instruction_list &ls) throw() {
        NUM_DBL_PATCH_INSTRUCTIONS.fetch_add(ls.length());
    }


    void perf::visit_dbl_stub(unsigned num) throw() {
        NUM_DBL_STUB_INSTRUCTIONS.fetch_add(num);
    }


    void perf::visit_mem_ref(unsigned num) throw() {
        NUM_MEM_REF_INSTRUCTIONS.fetch_add(num);
    }


    void perf::visit_align_nop(unsigned num) throw() {
        NUM_ALIGN_NOP_INSTRUCTIONS.fetch_add(num);
    }


#if GRANARY_IN_KERNEL
    void perf::visit_interrupt(void) throw() {
        NUM_INTERRUPTS.fetch_add(1);
    }

    void perf::visit_delayed_interrupt(void) throw() {
        NUM_DELAYED_INTERRUPTS.fetch_add(1);
    }


    void perf::visit_recursive_interrupt(void) throw() {
        NUM_RECURSIVE_INTERRUPTS.fetch_add(1);
    }


    unsigned long perf::num_delayed_interrupts(void) throw() {
        return NUM_DELAYED_INTERRUPTS.load();
    }

    void perf::visit_protected_module(void) throw() {
        NUM_BAD_MODULE_EXECS.fetch_add(1);
    }
#endif

    // If we're in the kernel, and regardless of
#if GRANARY_IN_KERNEL && defined(DETACH_ADDR_printk)
    extern "C" {
        extern int printk(const char *, ...);
    }
#   define printf printk
#endif

    void perf::report(void) throw() {

        printf("Number of decoded instructions: %u\n",
            NUM_DECODED_INSTRUCTIONS.load());
        printf("Number of decoded instruction bytes: %u\n\n",
            NUM_DECODED_BYTES.load());

        printf("Number of encoded instructions: %u\n",
            NUM_ENCODED_INSTRUCTIONS.load());
        printf("Number of encoded instruction bytes: %u\n\n",
            NUM_ENCODED_BYTES.load());

        printf("Number of basic blocks: %u\n",
            NUM_BBS.load());
        printf("Number of application instruction bytes: %u\n",
            NUM_BB_INSTRUCTION_BYTES.load());
        printf("Number of patch/stub instruction bytes: %u\n",
            NUM_BB_PATCH_BYTES.load());
        printf("Number of state bytes: %u\n\n",
            NUM_BB_STATE_BYTES.load());

        printf("Number of indirect JMPs: %u\n",
            NUM_INDIRECT_JMPS.load());
        printf("Number of indirect CALLs: %u\n",
            NUM_INDIRECT_CALLS.load());
        printf("Number of RETs: %u\n\n",
            NUM_RETURNS.load());

        printf("Number of entries in the global IBL hash table: %u\n",
            NUM_IBL_HTABLE_ENTRIES.load());
        printf("Number of misses in the IBL hash/jump table: %u\n",
            NUM_IBL_MISSES.load());
        printf("Number of conflicts in the IBL hash/jump table: %u\n\n",
            NUM_IBL_CONFLICTS.load());

        printf("Number of IBL entry instructions: %u\n",
            NUM_IBL_ENTRY_INSTRUCTIONS.load());
        printf("Number of IBL instructions: %u\n",
            NUM_IBL_INSTRUCTIONS.load());
        printf("Number of IBL exit instructions: %u\n\n",
            NUM_IBL_EXIT_INSTRUCTIONS.load());

        printf("Number of DBL entry instructions: %u\n",
            NUM_DBL_INSTRUCTIONS.load());
        printf("Number of DBL patch-stub instructions: %u\n",
            NUM_DBL_STUB_INSTRUCTIONS.load());
        printf("Number of DBL patch-setup instructions: %u\n\n",
            NUM_DBL_PATCH_INSTRUCTIONS.load());

        printf("Number of extra instructions to mangle memory refs: %u\n\n",
            NUM_MEM_REF_INSTRUCTIONS.load());
        printf("Number of alignment NOPs: %u\n\n",
            NUM_ALIGN_NOP_INSTRUCTIONS.load());

        printf("Number of global code cache address lookups: %u\n",
            NUM_ADDRESS_LOOKUPS.load());
        printf("Number hits in the global code cache: %u\n",
            NUM_ADDRESS_LOOKUP_HITS.load());
        printf("Number hits in the cpu private code cache(s): %u\n",
            NUM_ADDRESS_LOOKUPS_CPU_HIT.load());
        printf("Number misses in the cpu code cache(s): %u\n\n",
            NUM_ADDRESS_LOOKUPS_CPU_MISS.load());

#if GRANARY_IN_KERNEL
        printf("Number of interrupts: %lu\n",
            NUM_INTERRUPTS.load());
        printf("Number of delayed interrupts: %lu\n",
            NUM_DELAYED_INTERRUPTS.load());
        printf("Number of recursive interrupts (these are bad): %u\n",
            NUM_RECURSIVE_INTERRUPTS.load());
        printf("Number of interrupts due to insufficient wrapping: %lu\n\n",
            NUM_BAD_MODULE_EXECS.load());
#endif

        if(NUM_IBL_CONFLICTS.load()) {
            printf("IBL Targets:\n");
            char buff[200] = {'\0'};
            int j(0);

            const unsigned num_entries(NUM_IBL_HTABLE_ENTRIES.load());
            for(unsigned i(0); i < num_entries;) {
                if(!IBL_TARGETS[i].target) {
                    break;
                }
                j += sprintf(
                    &(buff[j]),
                    "(%p" IF_PROFILE_IBL(",%p,%lu,%lu") "),",
                    IF_PROFILE_IBL_((void *) IBL_TARGETS[i].source)
                    (void *) IBL_TARGETS[i].target
                    _IF_PROFILE_IBL(IBL_TARGETS[i].count[0], IBL_TARGETS[i].count[1]));

                ++i;
                if(!(i % IF_PROFILE_IBL_ELSE(2, 4))) {
                    printf("%s\n", buff);
                    j = 0;
                }
            }

            if(j) {
                printf("%s\n", buff);
            }

            printf("\n\n");
        }
    }
}


#endif /* CONFIG_ENABLE_PERF_COUNTS */

