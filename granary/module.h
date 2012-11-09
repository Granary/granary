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

/// Internal representation of a Linux kernel module.
struct kernel_module {

    /// Is this the Granary module?
    int is_granary;

    /// Initial entrypoints into the module.
    int (**init)(void);
    void (**exit)(void);

    /// Various module addresses.
    void *address;
    void *text_begin;
    void *text_end;

    /// The current module state.
    enum {
        KERNEL_MODULE_STATE_LIVE,
        KERNEL_MODULE_STATE_COMING,
        KERNEL_MODULE_STATE_GOING
    } state;

    /// The next module object in the list of module objects.
    struct kernel_module *next;
};

/// Notifier that a particular module's state has changed.
void notify_module_state_change(struct kernel_module *);

#ifdef __cplusplus
}
#endif

#endif /* Granary_MODULE_H_ */
