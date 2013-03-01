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

namespace granary {

    /// The kind of a prediction table.
    enum prediction_table_kind : uint8_t {
        PREDICT_SINGLE_OVERWRITE,

        // must be reclaimed.
        PREDICT_DEAD
    };


    /// Represents a source prediction entry. Source entries are negated
    /// addresses, so that they can be compared with addresses by using the
    /// LEA and JRCXZ instructions, which do not modify flags.
    struct prediction_entry_source {
        int64_t addr;

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
        app_pc dest;
    } __attribute__((packed));


    /// Represents a code cache prediction table header. The default prediction
    /// table structure
    struct prediction_table {

        /// The approximate number of reads of this prediction table by the
        /// code.
        uint32_t num_reads;

        enum {
            COLD_READ_COUNT = 10
        };

        /// Meta-data for
        char meta_data[
            sizeof(prediction_entry) - sizeof(num_reads) - 1
        ];

        /// The kind of this prediction table. (actually of type
        /// `prediction_table_kind`).
        uint8_t kind;


        /// The first slots in this prediction table.
        prediction_entry entry __attribute__((aligned (16)));

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

    } __attribute__((packed, aligned(16)));
}


#endif /* granary_PREDICT_H_ */
