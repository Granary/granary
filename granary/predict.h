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
        PREDICT_NULL                = 0,
        PREDICT_SINGLE_OVERWRITE    = 1,

        // must be reclaimed.
        PREDICT_DEAD
    };


    /// Represents a prediction entry.
    struct prediction_entry {
    public:
        app_pc source;
        app_pc dest;
    } __attribute__((packed));


    /// Represents a code cache prediction table header. The default prediction
    /// table structure
    struct prediction_table {

        /// Meta information.
        prediction_table_kind kind;
        char meta_data[sizeof(prediction_entry) - 1];

        /// The first slots in this prediction table.
        prediction_entry entry __attribute__((aligned (16)));

        /// Instrument this prediction table.
        __attribute__((hot, noinline))
        static void instrument(
            prediction_table **table,
            cpu_state_handle cpu,
            app_pc source,
            app_pc dest
        ) throw();

    } __attribute__((packed, aligned(16)));

}


#endif /* granary_PREDICT_H_ */
