/* ******************************************************************************
 * Copyright (c) 2010-2013 Google, Inc.  All rights reserved.
 * Copyright (c) 2010 Massachusetts Institute of Technology  All rights reserved.
 * Copyright (c) 2000-2010 VMware, Inc.  All rights reserved.
 * ******************************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* Copyright (c) 2003-2007 Determina Corp. */
/* Copyright (c) 2001-2003 Massachusetts Institute of Technology */
/* Copyright (c) 2000-2001 Hewlett-Packard Company */

/* file "mangle.c" */

#include "deps/dr/globals.h"
#include "arch.h"
#include "deps/dr/link.h"
#include "arch.h"
#include "instr.h"
//#include "instr_create.h"
#include "decode.h"
#include "decode_fast.h"
#ifdef STEAL_REGISTER
#include "steal_reg.h"
#endif

#ifdef RCT_IND_BRANCH
# include "deps/dr/rct.h" /* rct_add_rip_rel_addr */
#endif

#ifdef UNIX
#include <sys/syscall.h>
#endif

#include <string.h> /* for memset */

/* make code more readable by shortening long lines
 * we mark everything we add as a meta-instr to avoid hitting
 * client asserts on setting translation fields
 */
#define POST instrlist_meta_postinsert
#define PRE  instrlist_meta_preinsert

/***************************************************************************/

/* Convert a short-format CTI into an equivalent one using
 * near-rel-format.
 * Remember, the target is kept in the 0th src array position,
 * and has already been converted from an 8-bit offset to an
 * absolute PC, so we can just pretend instructions are longer
 * than they really are.
 */
 instr_t *
convert_to_near_rel_common(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr)
{
    int opcode = instr_get_opcode(instr);
    DEBUG_DECLARE(const instr_info_t * info = instr_get_instr_info(instr);)
    app_pc target = NULL;

    if (opcode == OP_jmp_short) {
        instr_set_opcode(instr, OP_jmp);
        return instr;
    }

    if (OP_jo_short <= opcode && opcode <= OP_jnle_short) {
        /* WARNING! following is OP_ enum order specific */
        instr_set_opcode(instr, opcode - OP_jo_short + OP_jo);
        return instr;
    }

    LOG(THREAD, LOG_INTERP, 1, "convert_to_near_rel: unknown opcode: %d %s\n",
        opcode, info->name);
    ASSERT_NOT_REACHED();      /* conversion not possible OR not a short-form cti */
    return instr;
}

instr_t *
convert_to_near_rel_meta(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr)
{
    return convert_to_near_rel_common(dcontext, ilist, instr);
}

void
convert_to_near_rel(dcontext_t *dcontext, instr_t *instr)
{
    convert_to_near_rel_common(dcontext, NULL, instr);
}


/***************************************************************************/

/***************************************************************************/

