/*
 * perf.h
 *
 *  Created on: 2013-02-13
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_PERF_H_
#define Granary_PERF_H_

#include "granary/globals.h"

#if CONFIG_ENABLE_PERF_COUNTS

namespace granary {

    /// Forward declarations.
    struct instruction;
    struct basic_block;

    struct perf {

        static void visit_decoded(instruction &) throw();
        static void visit_encoded(instruction &) throw();
        static void visit_encoded(basic_block &) throw();

        static void report(void) throw();
    };

}

#endif /* CONFIG_ENABLE_PERF_COUNTS */

#endif /* Granary_PERF_H_ */
