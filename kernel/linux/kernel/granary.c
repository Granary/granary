
#include <linux/granary.h>

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/seqlock.h>
#include <linux/lockdep.h>
#include <linux/completion.h>
#include <linux/debugobjects.h>
#include <linux/bug.h>
#include <linux/compiler.h>
#include <linux/export.h>


#define NO_INLINE_ASM \
    { \
        __asm__ __volatile__ (""); \
    }


noinline int __granary_rcu_compare_eq(const void *p1_, const void *p2_) {
    unsigned long p1 = (unsigned long) p1_;
    unsigned long p2 = (unsigned long) p2_;

    if(p1 == p2) {
        return 1;
    }

    if(!(p1 & (1ULL << 48)) && (p1 & (1ULL << 47))) {
        p1 |= (0xFFFFULL << 48);
    }

    if(!(p2 & (1ULL << 48)) && (p2 & (1ULL << 47))) {
        p2 |= (0xFFFFULL << 48);
    }

    return p1 == p2;
}
EXPORT_SYMBOL_GPL(__granary_rcu_compare_eq);


noinline void *__granary_rcu_dereference(
    void **p_orig,
    void *p_access,
    const char *carat
) {
    (void) p_orig;
    (void) carat;
    NO_INLINE_ASM;
    return p_access;
}
EXPORT_SYMBOL_GPL(__granary_rcu_dereference);


noinline void __granary_rcu_assign_pointer(
    void **ptr,
    void *val,
    const char *carat
) {
    smp_wmb();
    *ptr = val;
    smp_wmb();
}
EXPORT_SYMBOL_GPL(__granary_rcu_assign_pointer);


noinline void __granary_rcu_read_lock(
    enum granary_rcu_read_lock_type type,
    void *return_address,
    const char *carat
) {
    (void) type;
    (void) return_address;
    (void) carat;
    NO_INLINE_ASM;
}
EXPORT_SYMBOL_GPL(__granary_rcu_read_lock);


noinline void __granary_rcu_read_unlock(
    enum granary_rcu_read_lock_type type,
    void *return_address,
    const char *carat
) {
    (void) type;
    (void) return_address;
    (void) carat;
    NO_INLINE_ASM;
}
EXPORT_SYMBOL_GPL(__granary_rcu_read_unlock);
