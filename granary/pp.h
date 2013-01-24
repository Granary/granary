/*
 * pp.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_PP_H_
#define Granary_PP_H_


/// Used to denote entrypoints into Granary.
#define GRANARY_ENTRYPOINT

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

#define ALIGN_TO(lval, const_align) \
    (((lval) % (const_align)) ? ((const_align) - ((lval) % (const_align))) : 0)

#if GRANARY_IN_KERNEL
#   define IF_KERNEL(...) __VA_ARGS__
#   define IF_KERNEL_(...) , __VA_ARGS__
#   define IF_KERNEL_ELSE(if_true, if_false) if_true
#   define IF_KERNEL_ELSE_(if_true, if_false) , if_true
#   define IF_USER(...)
#   define IF_USER_(...)
#else
#   define IF_KERNEL(...)
#   define IF_KERNEL_(...)
#   define IF_KERNEL_ELSE(if_true, if_false) if_false
#   define IF_KERNEL_ELSE_(if_true, if_false) , if_false
#   define IF_USER(...) __VA_ARGS__
#   define IF_USER_(...) , __VA_ARGS__
#endif

#define FAULT (granary_break_on_fault(), granary_fault())
#define BARRIER ASM("" : : : "memory")

#define IF_DEBUG(cond, expr) {if(cond) { expr; }}

/// Use to statically initialise some code.
#define STATIC_INITIALISE___(id, ...) \
    static void init_func_ ## id(void) throw(); \
    struct init_class_ ## id : public granary::static_init_list { \
    public: \
        init_class_ ## id(void) throw() { \
            this->exec = init_func_ ## id; \
            granary::static_init_list::append(*this); \
        } \
    }; \
    static init_class_ ## id init_val_ ## id; \
    static __attribute__((noinline)) void init_func_ ## id(void) { \
        (void) init_val_ ## id; \
        { __VA_ARGS__ } \
    }

#define STATIC_INITIALISE__(line, counter, ...) \
    STATIC_INITIALISE___(line ## _ ## counter, ##__VA_ARGS__)
#define STATIC_INITIALISE_(line, counter, ...) \
    STATIC_INITIALISE__(line, counter, ##__VA_ARGS__)
#define STATIC_INITIALISE(...) \
    STATIC_INITIALISE_(__LINE__, __COUNTER__, ##__VA_ARGS__)

#if GRANARY_IN_KERNEL
#   define IF_TEST(...)
#   define ADD_TEST(func, desc)
#   define ASSERT(cond)
#else
#   define IF_TEST(...) __VA_ARGS__
#   define ADD_TEST(test_func, test_desc) \
    STATIC_INITIALISE({ \
        static granary::static_test_list test__; \
        test__.func = test_func; \
        test__.desc = test_desc; \
        granary::static_test_list::append(test__); \
    })
#   define ASSERT(cond) {if(!(cond)) { FAULT; }}
#endif

#define ASM(...) __asm__ __volatile__ ( __VA_ARGS__ )

/// unrolling macros for applying something to all general purpose registers
#define ALL_REGS(R, R_last) \
    R(rdi, R(rsi, R(rdx, R(rbx, R(rcx, R(rax, R(r8, R(r9, R(r10, R(r11, R(r12, R(r13, R(r14, R_last(r15))))))))))))))

/// unrolling macros for applying something to all argument registers
#define ALL_CALL_REGS(R, R_last) \
    R(rdi, R(rsi, R(rdx, R(rcx, R(r8, R(r9, R(rbp, R_last(rax))))))))


#define FOR_EACH_DIRECT_JUMP(macro, ...) \
    macro(jo, 3, ##__VA_ARGS__) \
    macro(jno, 4, ##__VA_ARGS__) \
    macro(jb, 3, ##__VA_ARGS__) \
    macro(jnb, 4, ##__VA_ARGS__) \
    macro(jz, 3, ##__VA_ARGS__) \
    macro(jnz, 4, ##__VA_ARGS__) \
    macro(jbe, 4, ##__VA_ARGS__) \
    macro(jnbe, 5, ##__VA_ARGS__) \
    macro(js, 3, ##__VA_ARGS__) \
    macro(jns, 4, ##__VA_ARGS__) \
    macro(jp, 3, ##__VA_ARGS__) \
    macro(jnp, 4, ##__VA_ARGS__) \
    macro(jl, 3, ##__VA_ARGS__) \
    macro(jnl, 4, ##__VA_ARGS__) \
    macro(jle, 4, ##__VA_ARGS__) \
    macro(jnle, 5, ##__VA_ARGS__) \
    macro(loop, 5, ##__VA_ARGS__) \
    macro(loopne, 7, ##__VA_ARGS__) \
    macro(loope, 6, ##__VA_ARGS__) \
    macro(jmp, 4, ##__VA_ARGS__) \
    macro(jmp_short, 10, ##__VA_ARGS__) \
    macro(jmp_ind, 8, ##__VA_ARGS__) \
    macro(jmp_far, 8, ##__VA_ARGS__) \
    macro(jmp_far_ind, 12, ##__VA_ARGS__) \
    macro(jecxz, 6, ##__VA_ARGS__)

#define TO_STRING_(x) #x
#define TO_STRING(x) TO_STRING_(x)


#define CAT__(x, y) x ## y
#define CAT_(x, y) CAT__(x, y)
#define CAT(x, y) CAT_(x, y)


#define NOTHING__
#define NOTHING_ NOTHING__
#define NOTHING NOTHING_

#define EACH(pp, ap, sep, ...) \
    CAT(EACH_, NUM_PARAMS(__VA_ARGS__))(pp, ap, sep, ##__VA_ARGS__)

#define EACH_0(pp, ap, sep,a0)

#define EACH_1(pp, ap, sep, a0) \
    CAT(CAT(pp, a0), ap)

#define EACH_2(pp, ap, sep, a0, a1) \
    EACH_1(pp, ap, sep, a0) sep CAT(CAT(pp, a1), ap)

#define EACH_3(pp, ap, sep, a0, a1, a2) \
    EACH_2(pp, ap, sep, a0, a1) sep CAT(CAT(pp, a2), ap)

#define EACH_4(pp, ap, sep, a0, a1, a2, a3) \
    EACH_3(pp, ap, sep, a0, a1, a2) sep CAT(CAT(pp, a3), ap)

#define EACH_5(pp, ap, sep, a0, a1, a2, a3, a4) \
    EACH_4(pp, ap, sep, a0, a1, a2, a3) sep CAT(CAT(pp, a4), ap)

#define EACH_6(pp, ap, sep, a0, a1, a2, a3, a4, a5) \
    EACH_5(pp, ap, sep, a0, a1, a2, a3, a4) sep CAT(CAT(pp, a5), ap)


/// Determine the number of arguments in a variadic macro argument pack.
/// Taken from: http://efesx.com/2010/07/17/variadic-macro-to-count-number-of-arguments/#comment-256
#define NUM_PARAMS(...) NUM_PARAMS_IMPL(, ##__VA_ARGS__,7,6,5,4,3,2,1,0)
#define NUM_PARAMS_IMPL(_0,_1,_2,_3,_4,_5,_6,_7,N,...) N


#define PARAMS(...) __VA_ARGS__


#define NOTHING__
#define NOTHING_ NOTHING__
#define NOTHING NOTHING_

#define TEMPLATE_PARAMS(...) \
    CAT(TEMPLATE_PARAMS_, NUM_PARAMS(__VA_ARGS__))(__VA_ARGS__)
#define TEMPLATE_PARAMS_0()
#define TEMPLATE_PARAMS_1(...) < __VA_ARGS__ >
#define TEMPLATE_PARAMS_2(...) < __VA_ARGS__ >
#define TEMPLATE_PARAMS_3(...) < __VA_ARGS__ >
#define TEMPLATE_PARAMS_4(...) < __VA_ARGS__ >
#define TEMPLATE_PARAMS_5(...) < __VA_ARGS__ >
#define TEMPLATE_PARAMS_6(...) < __VA_ARGS__ >
#define TEMPLATE_PARAMS_7(...) < __VA_ARGS__ >

#endif /* Granary_PP_H_ */
