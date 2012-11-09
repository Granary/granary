/*
 * module.h
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_MODULE_H_
#define Granary_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct kernel_module {
    int (**init)(void);
    void (**exit)(void);

    void *address;
    void *text_begin;
    void *text_end;

    enum {
        KERNEL_MODULE_STATE_LIVE,
        KERNEL_MODULE_STATE_COMING,
        KERNEL_MODULE_STATE_GOING
    } state;

    struct kernel_module *next;
};

void notify_module_state_change(struct kernel_module *);

#ifdef __cplusplus
}
#endif

#endif /* Granary_MODULE_H_ */
