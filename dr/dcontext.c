/*
 * dcontext.c
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "dr/globals.h"
#include "dr/types.h"

dcontext_t DCONTEXT = {
    .x86_mode = false,
    .private_code = NULL,
    .allocated_instr = NULL
};

dcontext_t *get_thread_private_dcontext(void) {
    return &DCONTEXT;
}
