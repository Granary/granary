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
            offsetof(T, entry) == sizeof(prediction_entry), \
            "The first slot of a prediction table must be at offset 16 (bytes)."); \
    };

namespace granary {


    /// Represents a prediction table that contains a single normal entry that
    /// is overwritten each time it is not matched.
    struct single_overwrite_prediction_table {
        prediction_table_kind kind;
        volatile unsigned num_overwrites;
        prediction_entry entry __attribute__((aligned (16)));
        prediction_entry ibl_entry __attribute__((aligned (16)));
    } __attribute__((packed));


    SANTIY_CHECK_PREDICT(prediction_table)
    SANTIY_CHECK_PREDICT(single_overwrite_prediction_table)


    /// Allocate a generic prediction table.
    template <typename T>
    __attribute__((hot))
    static T *make_table(
        cpu_state_handle &cpu,
        prediction_entry *fall_through,
        prediction_table_kind kind,
        unsigned num_extra_entries,
        unsigned last_entry
    ) throw() {
        T *table(unsafe_cast<T *>(
            cpu->block_allocator.allocate_untyped(16,
                sizeof(T) + sizeof(prediction_entry) * (num_extra_entries))));

        table->kind = kind;
        prediction_entry *entries(&(table->entry));
        memcpy(&(entries[last_entry]), fall_through, sizeof *fall_through);
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
        __sync_fetch_and_add(&(table->num_overwrites), 1);
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
        prediction_table *table(*table_ptr);
        switch(table->kind) {

        // This is a new prediction entry. We will upgrade it to a prediction
        // table with single overwrite capabilities.
        case PREDICT_NULL:
            replace_table(table_ptr, table,
                make_single_overwrite_table(
                    cpu, &(table->entry), source, dest));
            break;

        case PREDICT_SINGLE_OVERWRITE:
            update_single_overwrite_table(table_ptr, table, source, dest);
            break;

        default:
            return;
        }
    }


}

