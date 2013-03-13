/*
 * From: http://wiki.osdev.org/C++
 */

#ifndef _ICXXABI_H
#define _ICXXABI_H

#define ATEXIT_MAX_FUNCS    128

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned uarch_t;

struct atexit_func_entry_t
{
    /*
    * Each member is at least 4 bytes large. Such that each entry is 12bytes.
    * 128 * 12 = 1.5KB exact.
    **/
    void (*destructor_func)(void *);
    void *obj_ptr;
    void *dso_handle;
};


/// Register a destructor.
int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);


/// Invoke destructors.
void __cxa_finalize(void *f);


__extension__ typedef int __guard __attribute__((mode(__DI__)));

/// Guard the intiialisation of local static declarations.
int __cxa_guard_acquire(__guard *);
void __cxa_guard_release(__guard *);
void __cxa_guard_abort(__guard *);

#ifdef __cplusplus
};
#endif

#endif /* _ICXXABI_H */
