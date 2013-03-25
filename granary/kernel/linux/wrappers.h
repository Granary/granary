/*
 * wrappers.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_KERNEL_OVERRIDE_WRAPPERS_H_
#define granary_KERNEL_OVERRIDE_WRAPPERS_H_

#ifndef D
#   define D(...) __VA_ARGS__
#endif


#if defined(CAN_WRAP_kthread_create_on_node) && CAN_WRAP_kthread_create_on_node
#   define WRAPPER_FOR_kthread_create_on_node
    FUNCTION_WRAPPER(kthread_create_on_node, (struct task_struct *), (
        int (*threadfn)(void *),
        void *data,
        int node,
        const char namefmt[],
        ...
    ), {
        va_list args__;
        va_start(args__, namefmt);

        WRAP_FUNCTION(threadfn);
        char name_buff[sizeof(task_struct().comm)];
        vsnprintf(&(name_buff[0]), sizeof(name_buff), namefmt, args__);
        struct task_struct *ret = kthread_create_on_node(threadfn, data, node, name_buff);
        va_end(args__);
        RETURN_WRAP(ret);
        return ret;
    })
#endif




#endif /* granary_KERNEL_OVERRIDE_WRAPPERS_H_ */
