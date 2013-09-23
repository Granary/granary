/**
 * Definitions for Granary-specific functions and macros.
 *
 * Copyright 2013 Peter Goodman, all rights reserved.
 */

#ifndef LINUX_GRANARY_H_
#define LINUX_GRANARY_H_

#ifdef __LINUX_RCUPDATE_H
#   define GRANARY_EXTERN noinline extern
#else
#   define GRANARY_EXTERN
#endif

#define GRANARY_TO_STR__(x) #x
#define GRANARY_TO_STR_(x) GRANARY_TO_STR__(x)
#define GRANARY_TO_STR(x) GRANARY_TO_STR_(x)
#define GRANARY_SPLAT(x) x
#define GRANARY_CARAT GRANARY_SPLAT( __FILE__ ":" GRANARY_TO_STR(__LINE__) )


/**
 * Compare two potentially RCU-protected object pointers for equality.
 */
#define granary_rcu_compare_eq(a, b) \
    (__granary_rcu_compare_eq( \
        (const void *) (typeof(*(a)) *) (a), \
        (const void *) (typeof(*(b)) *) (b)))

#define granary_rcu_compare_neq(a, b) \
    (!(granary_rcu_compare_eq(a, b)))

GRANARY_EXTERN int __granary_rcu_compare_eq(const void *p1_, const void *p2_);


/**
 * De-reference an RCU-protected object.
 *
 * Example kernel code:
 *      p = rcu_dereference(q);
 */
#define granary_rcu_dereference(p_orig, p_access) \
    (__granary_rcu_dereference( \
        (void **) &(p_orig), \
        (void *) (p_access), \
        GRANARY_CARAT))


GRANARY_EXTERN void *__granary_rcu_dereference(void **, void *, const char *);


/**
 * Assign an RCU-protected object to a pointer.
 * Note: The usage of this is such that the pointer has not been assigned yet
 *       and cannot be assigned by `__granary_rcu_assign_pointer`.
 *
 * Example kernel code:
 *      rcu_assign_pointer(p, q);
 */
#define granary_rcu_assign_pointer(p, v) \
    (__granary_rcu_assign_pointer( \
        (void **) &(p), \
        (void *) (v), \
        GRANARY_CARAT))


GRANARY_EXTERN void __granary_rcu_assign_pointer(
    void **p,
    void *v,
    const char *
);


enum granary_rcu_read_lock_type {
    GRANARY_RCU_READ_LOCK_CLASSIC,
    GRANARY_RCU_READ_LOCK_BH,
    GRANARY_RCU_READ_LOCK_SCHED
};

/**
 * Mark the beginning of a read-critical section.
 */
#define granary_rcu_read_lock(__kind) \
    { \
        __granary_rcu_read_lock( \
            __kind, \
            (void *) __builtin_return_address(0), \
            GRANARY_CARAT); \
    }


GRANARY_EXTERN void __granary_rcu_read_lock(
    enum granary_rcu_read_lock_type,
    void *,
    const char *
);


/**
 * Mark the end of a read-critical section.
 *
 * Example kernel code:
 *      rcu_read_lock();
 */
#define granary_rcu_read_unlock(__kind) \
    { \
        __granary_rcu_read_unlock( \
            __kind, \
            (void *) __builtin_return_address(0), \
            GRANARY_CARAT); \
    }


GRANARY_EXTERN void __granary_rcu_read_unlock(
    enum granary_rcu_read_lock_type,
    void *,
    const char *
);

#endif /* LINUX_GRANARY_H_ */
