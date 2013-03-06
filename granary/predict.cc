/*
 * predict.cc
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <atomic>

#include "granary/predict.h"
#include "granary/globals.h"


/// Used to sanity check on the layout of a prediction table in memory.
#define SANTIY_CHECK_PREDICT(T) \
    struct sanity_check ## T { \
        static_assert( \
            offsetof(T, kind) == offsetof(prediction_table, kind), \
            "The `kind` field of `" #T "` must be at the same offset of the " \
            "`kind` field of `prediction_table`."); \
        static_assert( \
            offsetof(T, num_reads) == offsetof(prediction_table, num_reads), \
            "The `num_reads` field of `" #T "` must be at the same offset of the " \
            "`num_reads` field of `prediction_table`."); \
        static_assert( \
            offsetof(T, entry) == sizeof(prediction_entry), \
            "The first slot of a prediction table must be at offset 16 (bytes)."); \
        static_assert( \
            offsetof(T, num_reads) == 0, \
            "The `num_reads` field of `" #T "` must be at offset 0."); \
        static_assert( \
            sizeof(T().num_reads) == 4, \
            "The `num_reads` field of `" #T "` must be 4 bytes wide."); \
    };

namespace granary {


    /// Represents a prediction table that contains a single normal entry that
    /// is overwritten each time it is not matched.
    struct single_overwrite_prediction_table {
        uint32_t num_reads;
        uint32_t num_overwrites;

        char filler[sizeof(uint64_t) - sizeof(prediction_table_kind)];

        prediction_table_kind kind;

        prediction_entry entry;
        prediction_entry ibl_entry;

    } __attribute__((aligned(16)));


    SANTIY_CHECK_PREDICT(prediction_table)
    SANTIY_CHECK_PREDICT(single_overwrite_prediction_table)


    /// Allocate a generic prediction table.
    template <typename T>
    __attribute__((hot))
    static T *make_table(
        cpu_state_handle &cpu,
        prediction_entry *ibl_fall_through_entry,
        prediction_table_kind kind,
        unsigned num_extra_entries,
        unsigned last_entry_index
    ) throw() {
        T *table(unsafe_cast<T *>(
            cpu->small_allocator.allocate_untyped(16,
                sizeof(T) + sizeof(prediction_entry) * num_extra_entries)));

        table->kind = kind;
        prediction_entry *entries(&(table->entry));
        memcpy(
            &(entries[last_entry_index]),
            ibl_fall_through_entry,
            sizeof *ibl_fall_through_entry);
        return table;
    }


    /// Create a single overwrite entry prediction table.
    __attribute__((hot))
    static prediction_table *make_single_overwrite_table(
        cpu_state_handle &cpu,
        prediction_entry *fall_through,
        app_pc source,
        app_pc dest
    ) throw() {
        single_overwrite_prediction_table *table(
            make_table<single_overwrite_prediction_table>(
                cpu, fall_through, PREDICT_SINGLE_OVERWRITE, 0, 1));
        table->entry.source = source;
        table->entry.dest = dest;
        return unsafe_cast<prediction_table *>(table);
    }


    /// Update a single overwrite entry prediction table.
    __attribute__((hot))
    static void update_single_overwrite_table(
        prediction_table **table_ptr,
        prediction_table *table_,
        app_pc source,
        app_pc dest
    ) throw() {
        single_overwrite_prediction_table *table(
            unsafe_cast<single_overwrite_prediction_table *>(table_));

        // beware racy code below!
        table->num_overwrites += 1;
        table->num_reads = 0;

        // TODO: race condition
        table->entry.source = source;
        table->entry.dest = dest;

        // TODO
        (void) source;
        (void) dest;
        (void) table_ptr;
    }


    /// Replace one prediction table with another.
    __attribute__((hot))
    static void replace_table(
        prediction_table **loc,
        prediction_table *old,
        prediction_table *new_
    ) throw() {
        std::atomic_thread_fence(std::memory_order_release);
        *loc = new_;
        old->kind = PREDICT_DEAD;
    }

#define DEBUG_PREDICT_TABLE_CORRUPTION 0
#if DEBUG_PREDICT_TABLE_CORRUPTION
    static hash_table<app_pc, prediction_table *> pt;
#endif

    /// Returns the default table for some IBL.
    prediction_table *prediction_table::get_default(
        cpu_state_handle &cpu,
        app_pc ibl
    ) throw() {
        prediction_entry null_entry = {nullptr, ibl};
#if DEBUG_PREDICT_TABLE_CORRUPTION
        if(!pt.find(ibl)) {
            prediction_table *e(make_single_overwrite_table(
                cpu, &null_entry, nullptr, ibl));

            pt.store(ibl, e);
        }
        return pt.find(ibl);
#endif
        return make_single_overwrite_table(
            cpu, &null_entry, nullptr, ibl);
    }


    /// Instrument an indirect branch lookup that failed to match `source` to
    /// a known destination address in its current table.
    ///
    /// Here we define the policies on how to manage a prediction table.
    void prediction_table::instrument(
        prediction_table **table_ptr,
        cpu_state_handle &cpu,
        app_pc source,
        app_pc dest
    ) throw() {
        (void) cpu;
        prediction_table *table(*table_ptr);
        switch(table->kind) {

        case PREDICT_SINGLE_OVERWRITE:
            update_single_overwrite_table(table_ptr, table, source, dest);
            break;

        default:
            return;
        }
    }


}

