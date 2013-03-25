/*
 * predict.cc
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <atomic>

#include "granary/predict.h"
#include "granary/globals.h"
#include "granary/smp/spin_lock.h"

#if CONFIG_ENABLE_IBL_PREDICTION_STUBS

#define D(...)


extern "C" {
    __attribute__((noinline))
    void granary_break_on_predict(void) {
        ASM("");
    }
}


/// Used to sanity check on the layout of a prediction table in memory.
#define SANTIY_CHECK_PREDICT(T) \
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

namespace granary {

    /// Check that the prediction table structures have the same general layout
    /// in the places where it matters.
    SANTIY_CHECK_PREDICT(prediction_table)


    /// Allocate a generic prediction table.
    template <typename T>
    __attribute__((hot))
    static T *make_table(
        cpu_state_handle &cpu,
        const prediction_entry *ibl_fall_through_entry,
        const prediction_table_kind kind,
        const unsigned last_entry_index
    ) throw() {

        static_assert(0 == (sizeof(T) % 16),
            "Invalid template argument `T` to `make_table`.");

        T *table(unsafe_cast<T *>(
            cpu->small_allocator.allocate_untyped(16, sizeof(T))));

        table->kind = kind;
        prediction_entry *entries(&(table->entry));
        for(unsigned i(0); i <= last_entry_index; ++i) {
            memcpy(
                &(entries[i]),
                ibl_fall_through_entry,
                sizeof *ibl_fall_through_entry
            );
        }

        return table;
    }


#if CONFIG_ENABLE_IBL_LINEAR_TABLE

    /// Represents a table with a fixed number of slots that are overwritten
    /// once and then remain fixed.
    struct linear_prediction_table {
        volatile uint32_t num_reads;
        volatile uint32_t num_misses;
        volatile uint16_t num_total_entries;
        volatile uint16_t num_used_entries;

        smp::atomic_spin_lock update_lock;

        uint8_t scale_level;

        char filler[
            sizeof(uint32_t) - sizeof(prediction_table_kind)
                             - sizeof(smp::atomic_spin_lock)
                             - sizeof(uint8_t)
        ];

        volatile prediction_table_kind kind;

        prediction_entry entry;

    } __attribute__((aligned(16)));


    /// Represents one of the linear prediction table kinds.
    template <const unsigned num_entries>
    struct linear_prediction_table_ext : public linear_prediction_table {

        prediction_entry entries_rest[num_entries - 1];

        const prediction_entry ibl_entry;


        linear_prediction_table_ext(void) throw()
            : ibl_entry()
        { FAULT; }

    } __attribute__((aligned(16)));


    SANTIY_CHECK_PREDICT(linear_prediction_table)


    /// Create a (scale 1) linear prediction table.
    __attribute__((hot))
    static prediction_table *make_linear_table(
        cpu_state_handle &cpu,
        const prediction_entry *ibl_entry
    ) throw() {
        linear_prediction_table_ext<PREDICT_LINEAR_1> *table(
            make_table<linear_prediction_table_ext<PREDICT_LINEAR_1>>(
                cpu, ibl_entry, PREDICT_LINEAR, PREDICT_LINEAR_1));
        table->num_total_entries = PREDICT_LINEAR_1;
        table->scale_level = 1;
        return unsafe_cast<prediction_table *>(table);
    }


    const static uint16_t num_linear_entries[] = {
        0,
        PREDICT_LINEAR_1,
        PREDICT_LINEAR_2,
        PREDICT_LINEAR_3
    };


    /// Decide how to replace an overwrite prediction table.
    __attribute__((hot))
    static void replace_linear_table(
        cpu_state_handle &cpu,
        prediction_table **table_ptr,
        const app_pc source,
        const app_pc dest,
        const unsigned reads_to_overwrites,
        const unsigned num_entries
    ) throw() {
        const prediction_table *table(*table_ptr);
        prediction_table *new_table(nullptr);
        linear_prediction_table_ext<PREDICT_LINEAR_1> *table_2(nullptr);
        prediction_entry *new_entries(nullptr);
        const prediction_entry *old_entries(&(table->entry));
        const prediction_entry *ibl_entry(&(old_entries[num_entries]));

        switch(reads_to_overwrites) {

        // nothing (so far) is a good predictor
        case 0:
            goto no_good_predictions;

        // potentially one of two things are a good predictor
        case 1:
            table_2 = make_table<linear_prediction_table_ext<PREDICT_LINEAR_1>>(
                cpu, ibl_entry, PREDICT_LINEAR, PREDICT_LINEAR_1);
            goto initialize_linear_table;

        // potentially one of four things are a good predictor
        case 2:
            table_2 = unsafe_cast<linear_prediction_table_ext<PREDICT_LINEAR_1> *>(
                make_table<linear_prediction_table_ext<PREDICT_LINEAR_2>>(
                    cpu, ibl_entry, PREDICT_LINEAR, PREDICT_LINEAR_2));
            goto initialize_linear_table;

        // potentially one of eight things are a good predictor:
        case 3:
            table_2 = unsafe_cast<linear_prediction_table_ext<PREDICT_LINEAR_1> *>(
                make_table<linear_prediction_table_ext<PREDICT_LINEAR_3>>(
                    cpu, ibl_entry, PREDICT_LINEAR, PREDICT_LINEAR_3));

        initialize_linear_table:

            // fill in the entries
            new_entries = &(table_2->entry);
            for(unsigned i(0); i < num_entries; ++i) {
                memcpy(
                    &(new_entries[i]), &(old_entries[i]), sizeof table->entry);
            }
            new_entries[num_entries].source = source;
            new_entries[num_entries].dest = dest;

            table_2->num_total_entries = num_linear_entries[reads_to_overwrites];
            table_2->num_used_entries = num_entries + 1;
            table_2->scale_level = reads_to_overwrites;
            new_table = unsafe_cast<prediction_table *>(table_2);
            break;

        // unknown, just like case 0.
        no_good_predictions:
        default:
            // TODO
            granary_break_on_predict();
            break;
        }

        if(new_table) {
            std::atomic_thread_fence(std::memory_order_release);
            *table_ptr = new_table;
        }
    }


    /// Update a linear prediction table.
    __attribute__((hot))
    static void update_linear_table(
        cpu_state_handle &cpu,
        prediction_table **table_ptr,
        prediction_table *table_,
        app_pc source,
        app_pc dest
    ) {
        linear_prediction_table *table(
            unsafe_cast<linear_prediction_table *>(table_));

        table->update_lock.acquire();

        if(PREDICT_DEAD == table->kind) {
            table->update_lock.release();
            return;
        }

        // false miss
        const unsigned i(table->num_used_entries);
        if(i < table->num_total_entries) {

            table->num_used_entries += 1;
            table->num_reads = 0;

            prediction_entry *entries(&(table->entry));
            entries[i].source = source;
            std::atomic_thread_fence(std::memory_order_release);
            entries[i].dest = dest;

        // true miss; replace the table
        } else {
            table->num_misses += 1;
            table->kind = PREDICT_DEAD;
            replace_linear_table(
                cpu,
                table_ptr,
                source,
                dest,
                table->scale_level + 1,
                table->num_total_entries
            );
        }

        table->update_lock.release();
    }
#endif /* CONFIG_ENABLE_IBL_LINEAR_TABLE */


#if CONFIG_ENABLE_IBL_OVERWRITE_TABLE

    /// Represents a prediction table that contains a single normal entry that
    /// is overwritten each time it is not matched.
    struct overwrite_prediction_table {
        volatile uint32_t num_reads;
        volatile uint32_t num_overwrites;
        volatile uint32_t total_num_reads;

        smp::atomic_spin_lock overwrite_lock;

        char filler[
            sizeof(uint32_t) - sizeof(prediction_table_kind)
                             - sizeof(smp::atomic_spin_lock)
        ];

        volatile prediction_table_kind kind;

        prediction_entry entry;
        const prediction_entry ibl_entry;

        overwrite_prediction_table(void) throw()
            : ibl_entry()
        { FAULT; }

    } __attribute__((aligned(16)));


    SANTIY_CHECK_PREDICT(overwrite_prediction_table)


    /// Create a single overwrite entry prediction table.
    __attribute__((hot))
    static prediction_table *make_overwrite_table(
        cpu_state_handle &cpu,
        const prediction_entry *ibl_entry
    ) throw() {
        overwrite_prediction_table *table(
            make_table<overwrite_prediction_table>(
                cpu, ibl_entry, PREDICT_OVERWRITE, 1));
        return unsafe_cast<prediction_table *>(table);
    }


    /// Update an overwrite prediction table.
    __attribute__((hot))
    static void update_overwrite_table(
        cpu_state_handle &cpu,
        prediction_table **table_ptr,
        prediction_table *table_,
        app_pc source,
        app_pc dest
    ) throw() {
        overwrite_prediction_table *table(
            unsafe_cast<overwrite_prediction_table *>(table_));

        table->overwrite_lock.acquire();

        if(PREDICT_DEAD == table->kind) {
            table->overwrite_lock.release();
            return;
        }

        const unsigned old_num_reads(table->num_reads);
        const unsigned num_overwrites(table->num_overwrites += 1);
        const unsigned total_num_reads(table->total_num_reads += old_num_reads);

#if CONFIG_ENABLE_IBL_LINEAR_TABLE
        // should we change the prediction table type?
        if(prediction_table::HOT_OVERWRITE_COUNT <= num_overwrites) {

            // kill the old table and replace it.
            table->kind = PREDICT_DEAD;
            replace_linear_table(
                cpu,
                table_ptr,
                source,
                dest,
                (total_num_reads + 1) / (num_overwrites + 1),
                1
            );

        } else {
#else
        {
            (void) num_overwrites;
            (void) total_num_reads;
            (void) cpu;
            (void) table_ptr;
#endif /* CONFIG_ENABLE_IBL_LINEAR_TABLE */

            table->num_reads = 0;

            // overwrite the source and destination in three steps.
            table->entry.dest = table->ibl_entry.dest;
            std::atomic_thread_fence(std::memory_order_release);
            table->entry.source = source;
            std::atomic_thread_fence(std::memory_order_release);
            table->entry.dest = dest;
        }

        table->overwrite_lock.release();
    }

#endif /* CONFIG_ENABLE_IBL_OVERWRITE_TABLE */


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
            prediction_table *e(make_overwrite_table(
                cpu, &null_entry, nullptr, ibl));

            pt.store(ibl, e);
        }
        return pt.find(ibl);
#endif

#if CONFIG_ENABLE_IBL_OVERWRITE_TABLE
        return make_overwrite_table(cpu, &null_entry);
#endif
#if CONFIG_ENABLE_IBL_LINEAR_TABLE
        return make_linear_table(cpu, &null_entry);
#endif
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

#if CONFIG_ENABLE_IBL_OVERWRITE_TABLE
        case PREDICT_OVERWRITE:
            update_overwrite_table(cpu, table_ptr, table, source, dest);
            break;
#endif /* CONFIG_ENABLE_IBL_OVERWRITE_TABLE */
#if CONFIG_ENABLE_IBL_LINEAR_TABLE
        case PREDICT_LINEAR:
            update_linear_table(cpu, table_ptr, table, source, dest);
            break;
#endif /* CONFIG_ENABLE_IBL_OVERWRITE_TABLE */
        default:
            return;
        }
    }
}

#endif /* CONFIG_ENABLE_IBL_PREDICTION_STUBS */

