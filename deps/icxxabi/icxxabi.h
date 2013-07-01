/*
 * From: http://wiki.osdev.org/C++
 */

#ifndef _ICXXABI_H
#define _ICXXABI_H

#define ATEXIT_MAX_FUNCS    128

#ifdef __cplusplus
extern "C" {
#endif

#if 0
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

/// Invoke destructors.
void __cxa_finalize(void *f);

#endif

/// Register a destructor.
///
/// TODO: See the `-fuse-cxa-atexit` GCC compiler flag.
int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);

__extension__ typedef int __guard __attribute__((mode(__DI__)));

/// Guard the intiialisation of local static declarations.
int __cxa_guard_acquire(__guard *);
void __cxa_guard_release(__guard *);
void __cxa_guard_abort(__guard *);
void __cxa_pure_virtual(void);

#ifdef __cplusplus
};
#endif

#endif /* _ICXXABI_H */
