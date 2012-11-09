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
    .x86_mode = true,
    .private_code = NULL
};

dcontext_t *get_thread_private_dcontext(void) {
    return &DCONTEXT;
}
