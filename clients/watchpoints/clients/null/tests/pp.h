/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * pp.h
 *
 *  Created on: 2013-04-28
 *      Author: Peter Goodman
 */

#ifndef WP_TEST_PP_H_
#define WP_TEST_PP_H_


#define PUSH_LAST_REG(reg) \
    "push %" #reg ";"

#define PUSH_REG(reg, rest) \
    PUSH_LAST_REG(reg) \
    rest

#define POP_LAST_REG(reg) \
    "pop %" #reg ";"

#define POP_REG(reg, rest) \
    rest \
    POP_LAST_REG(reg)

#define PUSHA ALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA ALL_REGS(POP_REG, POP_LAST_REG)

#endif /* WP_TEST_PP_H_ */
