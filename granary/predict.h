/*
 * predict.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_PREDICT_H_
#define granary_PREDICT_H_

#include <cstddef>

#include "granary/state.h"

#if CONFIG_ENABLE_IBL_PREDICTION_STUBS

namespace granary {

    /// The kind of a prediction table.
    enum prediction_table_kind : uint8_t {

#if CONFIG_ENABLE_IBL_OVERWRITE_TABLE
        /// Prediction table with one "useful" entry that is constantly
        /// overwritten.
        PREDICT_OVERWRITE,
#endif

#if CONFIG_ENABLE_IBL_LINEAR_TABLE
        /// Prediction table with a fixed number of entry slots, which are
        /// filled out. Once filled, the entries don't change.
        PREDICT_LINEAR,
#endif

        /// A "dead" (i.e. unused) prediction table.
        ///
        /// TODO: Find a way to reclaim prediction tables.
        PREDICT_DEAD
    };


    /// The number of prediction entries for the first three levels of
    /// a linear prediction table.
    enum {
        PREDICT_LINEAR_1 = 2,
        PREDICT_LINEAR_2 = 4,
        PREDICT_LINEAR_3 = 8
    };


    /// Represents a source prediction entry. Source entries are negated
    /// addresses, so that they can be compared with addresses by using the
    /// LEA and JRCXZ instructions, which do not modify flags.
    struct prediction_entry_source {
        volatile int64_t addr;

        inline prediction_entry_source(void) throw()
            : addr(0)
        { }

        inline prediction_entry_source(app_pc addr_) throw()
            : addr(-reinterpret_cast<int64_t>(addr_))
        { }

        inline prediction_entry_source &
        operator=(app_pc addr_) throw() {
            addr = -reinterpret_cast<int64_t>(addr_);
            return *this;
        }

        inline operator app_pc(void) const throw() {
            return reinterpret_cast<app_pc>(-addr);
        }
    };


    /// Represents a prediction entry.
    struct prediction_entry {
    public:
        prediction_entry_source source;
        volatile app_pc dest;
    } __attribute__((aligned(16)));


    /// Sanity check the packing, as we can't pack it because
    /// prediction_entry_source is not a POD.
    static_assert(0 == offsetof(prediction_entry, source),
        "`source` field of `struct prediction_entry` was not placed correctly.");

    static_assert(8 == offsetof(prediction_entry, dest),
        "`dest` field of `struct prediction_entry` was not placed correctly.");

    static_assert(16 == sizeof(prediction_entry),
        "`struct prediction_entry` must be 16 bytes.");

    /// Represents a code cache prediction table header. The default prediction
    /// table structure
    struct prediction_table {

        /// The approximate number of reads of this prediction table by the
        /// code.
        uint32_t num_reads;

        enum {
            HOT_OVERWRITE_COUNT = 4
        };

        /// Meta-data for
        char meta_data[
            sizeof(prediction_entry) - sizeof(num_reads) - 1
        ];

        /// The kind of this prediction table. (actually of type
        /// `prediction_table_kind`).
        uint8_t kind;

        /// The first slots in this prediction table.
        prediction_entry entry;

        /// Instrument this prediction table.
        __attribute__((hot))
        static void instrument(
            prediction_table **table,
            cpu_state_handle &cpu,
            app_pc source,
            app_pc dest
        ) throw();


        /// Returns the default table for some IBL.
        static prediction_table *get_default(
            cpu_state_handle &cpu,
            app_pc ibl
        ) throw();

    } __attribute__((aligned(16)));
}

#endif /* CONFIG_ENABLE_IBL_PREDICTION_STUBS */

#endif /* granary_PREDICT_H_ */
