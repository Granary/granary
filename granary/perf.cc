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

    void perf::visit_decoded(instruction &in) throw() {
        NUM_DECODED_INSTRUCTIONS.fetch_add(1);
        NUM_DECODED_BYTES.fetch_add(in.instr.length);
    }


    void perf::visit_encoded(instruction &in) throw() {
        NUM_ENCODED_INSTRUCTIONS.fetch_add(1);
        NUM_ENCODED_BYTES.fetch_add(in.encoded_size());
    }


    void perf::visit_encoded(basic_block &bb) throw() {
        NUM_BBS.fetch_add(1);
        NUM_BB_INSTRUCTION_BYTES.fetch_add(
            bb.info->num_bytes - bb.info->num_patch_bytes);
        NUM_BB_PATCH_BYTES.fetch_add(bb.info->num_patch_bytes);
        NUM_BB_STATE_BYTES.fetch_add(sizeof(basic_block_state));
    }


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


    }
}


#endif /* CONFIG_ENABLE_PERF_COUNTS */

