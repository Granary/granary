/*
 * test_mat_mul.cc
 *
 *  Created on: 2013-02-05
 *      Author: pag
 */

#include <cstdlib>

#include "granary/test.h"

namespace test {

    enum {
        DIM = 10
    };

    static void multiply(
        int (&x)[DIM][DIM],
        int (&y)[DIM][DIM],
        int (&z)[DIM][DIM]
    ) {
        for(int i(0); i < DIM; ++i) {
            for(int j(0); j < DIM; ++j) {
                z[i][j] = 0;
            }
        }
        for(int i(0); i < DIM; ++i) {
            for(int j(0); j < DIM; ++j) {
                for(int k(0); k < DIM; ++k) {
                    z[i][k] += x[i][j] * y[j][k];
                }
            }
        }
    }

    static void fill_array(int (&x)[DIM][DIM]) {
        for(int i(0); i < DIM; ++i) {
            for(int j(0); j < DIM; ++j) {
                x[i][j] = rand();
            }
        }
    }

    static void test_mat_mul(void) {
        granary::app_pc func((granary::app_pc) multiply);
        granary::basic_block bb_func(granary::code_cache::find(
            func, granary::test_policy()));

        int x[DIM][DIM];
        int y[DIM][DIM];
        int z_native[DIM][DIM];
        int z_instrumented[DIM][DIM];

        fill_array(x);
        fill_array(y);

        bb_func.call<
            void,
            int (&)[DIM][DIM],
            int (&)[DIM][DIM],
            int (&)[DIM][DIM]
        >(x, y, z_instrumented);

        multiply(x, y, z_native);

        ASSERT(0 == memcmp(
            &(z_instrumented[0][0]),
            &(z_native[0][0]),
            DIM * DIM * sizeof(int)));
    }


    ADD_TEST(test_mat_mul,
        "Test that simple matrix multiplication works correctly.")

}

