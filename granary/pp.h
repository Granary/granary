/*
 * pp.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_PP_H_
#define Granary_PP_H_

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#   if __GNUC__ >= 4 && __GNUC_MINOR__ >= 7
#       define FORCE_INLINE inline
#   else
#       define FORCE_INLINE __attribute__((always_inline))
#   endif
#elif defined(__clang__)
#   define FORCE_INLINE __attribute__((always_inline))
#else
#   define FORCE_INLINE inline
#endif

#define GRANARY

#define ALIGN_TO(lval, const_align) (((lval) % (const_align)) ? ((const_align) - ((lval) % (const_align))) : 0)

#if GRANARY_IN_KERNEL
#   define IF_KERNEL(x) x
#   define IF_KERNEL_(x) , x
#   define IF_KERNEL_ELSE(if_true, if_false) if_true
#   define IF_KERNEL_ELSE_(if_true, if_false) , if_true
#   define IF_USER(x)
#   define IF_USER_(x)
#else
#   define IF_KERNEL(x)
#   define IF_KERNEL_(x)
#   define IF_KERNEL_ELSE(if_true, if_false) if_false
#   define IF_KERNEL_ELSE_(if_true, if_false) , if_false
#   define IF_USER(x) x
#   define IF_USER_(x) , x
#endif

#define FAULT (break_before_fault(), (*((int *) nullptr) = 0))
#define BARRIER __asm__ __volatile__ ("")

#define IF_DEBUG(cond, expr) {if(cond) { expr; }}


/// Use to statically initialize some code.
#define STATIC_INITIALIZE___(code, id) \
    static void foo_func_ ## id(void) throw(); \
    struct static_init_ ## id : public static_init_list { \
    public: \
        static_init_ ## id(void) throw() { \
            code \
            (void) foo_func_ ## id; \
            this->next = STATIC_LIST_HEAD.next; \
            STATIC_LIST_HEAD.next = this; \
        } \
    }; \
    static static_init_ ## id foo_ ## id; \
    static __attribute__((noinline)) void foo_func_ ## id(void) { (void) foo_ ## id; }

#define STATIC_INITIALIZE__(code, line, counter) STATIC_INITIALIZE___(code, line ## _ ## counter)
#define STATIC_INITIALIZE_(code, line, counter) STATIC_INITIALIZE__(code, line, counter)
#define STATIC_INITIALIZE(code) STATIC_INITIALIZE_(code, __LINE__, __COUNTER__)


#define TEST(code) code


#define ASM(code) __asm__ __volatile__ ( code )

#endif /* Granary_PP_H_ */
