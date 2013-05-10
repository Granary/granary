/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrapper.h
 *
 *      Author: Peter Goodman
 */

#ifndef granary_WRAPPER_H_
#define granary_WRAPPER_H_

#include "granary/globals.h"
#include "granary/detach.h"

#if CONFIG_ENABLE_WRAPPERS

#define P(...)
//__VA_ARGS__

namespace granary {



#if GRANARY_IN_KERNEL
    template <enum function_wrapper_id>
    struct kernel_address;

#   define DETACH(func)
#   define TYPED_DETACH(func)
#   define WRAP_FOR_DETACH(func) \
    template <> \
    struct kernel_address< DETACH_ID_ ## func > { \
    public: \
        enum { \
            VALUE = DETACH_ADDR_ ## func \
        }; \
    };
#   if GRANARY_IN_KERNEL
#       include "granary/gen/kernel_detach.inc"
#   else
#       include "granary/gen/user_detach.inc"
#   endif
#   undef DETACH
#   undef TYPED_DETACH
#   undef WRAP_FOR_DETACH
#endif


    enum {
        PRE_WRAP_MASK       = (1 << 0),
        POST_WRAP_MASK      = (1 << 1),
        RETURN_WRAP_MASK    = (1 << 2)
    };


    /// Base of a generic, unwrapped type. This exists to implement "null"
    /// functionality so that not all type wrappers need to specify pre/post/
    /// return wrappers.
    template <typename T>
    struct type_wrapper_base {
    public:

        inline static void pre_in_wrap(T &, const int /* depth__ */) throw() { }
        inline static void post_in_wrap(T &, const int /* depth__ */) throw() { }
        inline static void return_in_wrap(T &, const int /* depth__ */) throw() { }

        inline static void pre_out_wrap(T &, const int /* depth__ */) throw() { }
        inline static void post_out_wrap(T &, const int /* depth__ */) throw() { }
        inline static void return_out_wrap(T &, const int /* depth__ */) throw() { }
    };


    /// Represents a generic, unwrapped type.
    template <typename T>
    struct type_wrapper : public type_wrapper_base<T> {
    public:
        enum {

            /// Does this type have any wrappers? This will be a bitmask of
            /// the pre/post/return wrappers, so we can detect which are
            /// present.
            HAS_IN_WRAPPER          = 0,
            HAS_OUT_WRAPPER         = 0,

            /// Is a function pointer within T being used to track things (such
            /// as preventing re-wrapping)?
            HAS_META_INFO_TRACKER   = 0
        };
    };


#define WRAP_QUALIFIED_VALUE(prefix, qual, suffix, cast) \
    inline static void CAT(prefix, _wrap)( \
        qual T suffix&val, \
        const int depth \
    ) throw() { \
        type_wrapper<T suffix>::CAT(prefix, _wrap)( \
            cast<T suffix&>(val), depth); \
    }


/// Specialisations to handle qualified types.
#define WRAP_QUALIFIER_TYPE_IMPL(qual, suffix, cast) \
    template <typename T> \
    struct type_wrapper_base<qual T suffix > { \
    public: \
        WRAP_QUALIFIED_VALUE(pre_in, qual, suffix, cast) \
        WRAP_QUALIFIED_VALUE(pre_out, qual, suffix, cast) \
        WRAP_QUALIFIED_VALUE(post_in, qual, suffix, cast) \
        WRAP_QUALIFIED_VALUE(post_out, qual, suffix, cast) \
        WRAP_QUALIFIED_VALUE(return_in, qual, suffix, cast) \
        WRAP_QUALIFIED_VALUE(return_out, qual, suffix, cast) \
    };


#define WRAP_QUALIFIER_TYPE(qual, cast) \
    WRAP_QUALIFIER_TYPE_IMPL(qual, NOTHING, cast) \
    WRAP_QUALIFIER_TYPE_IMPL(qual, *, cast)

    /// Unwrap const and volatile types.
    WRAP_QUALIFIER_TYPE(const, const_cast)
    WRAP_QUALIFIER_TYPE(volatile, reinterpret_cast)


#if GRANARY_IN_KERNEL
    template <typename T>
    FORCE_INLINE
    static bool is_valid_address(T *addr) throw() {
        // Taken from Documentation/x86/x86_64/mm.txt
        return ((uintptr_t) addr) > 0x00007fffffffffff;
    }
#else
#   define is_valid_address(x) (4095 < ((uintptr_t) x))
#endif


    /// Defines a wrapper function for a pointer of type T.
#define WRAP_POINTER_VALUE(prefix) \
    inline static void CAT(prefix, _wrap)(T *&val, const int depth) throw() { \
        if(is_valid_address(val)) { \
            return; \
        } \
        type_wrapper<T>::CAT(prefix, _wrap)(*val, depth); \
    }


    /// Wrap generic pointers.
    template <typename T>
    struct type_wrapper_base<T *> {
    public:
        WRAP_POINTER_VALUE(pre_in)
        WRAP_POINTER_VALUE(pre_out)
        WRAP_POINTER_VALUE(post_in)
        WRAP_POINTER_VALUE(post_out)
        WRAP_POINTER_VALUE(return_in)
        WRAP_POINTER_VALUE(return_out)
    };


    /// Check to see if something, or some derivation of that thing, has a
    /// wrapper.
    template <typename T>
    struct has_in_wrapper {
    public:
        enum {
            VALUE = type_wrapper<T>::HAS_IN_WRAPPER
        };
    };


    template <typename T>
    struct has_in_wrapper<T *> {
    public:
        enum {
            VALUE = has_in_wrapper<T>::VALUE | type_wrapper<T *>::HAS_IN_WRAPPER
        };
    };


    template <typename T>
    struct has_in_wrapper<const T> {
    public:
        enum {
            VALUE = has_in_wrapper<T>::VALUE
                  | type_wrapper<const T>::HAS_IN_WRAPPER
        };
    };


    template <typename T>
    struct has_out_wrapper {
    public:
        enum {
            VALUE = type_wrapper<T>::HAS_OUT_WRAPPER
        };
    };


    template <typename T>
    struct has_out_wrapper<T *> {
    public:
        enum {
            VALUE = has_out_wrapper<T>::VALUE
                  | type_wrapper<T *>::HAS_OUT_WRAPPER
        };
    };


    template <typename T>
    struct has_out_wrapper<const T> {
    public:
        enum {
            VALUE = has_out_wrapper<T>::VALUE
                  | type_wrapper<const T>::HAS_OUT_WRAPPER
        };
    };


    /// Check to see if a derivation of that type has a wrapper.
    template <typename T>
    struct next_has_in_wrapper {
    public:
        enum {
            VALUE = 0
        };
    };


    template <typename T>
    struct next_has_in_wrapper<T *> {
    public:
        enum {
            VALUE = has_in_wrapper<T>::VALUE
        };
    };


    template <typename T>
    struct next_has_in_wrapper<const T> {
    public:
        enum {
            VALUE = has_in_wrapper<T>::VALUE
        };
    };


    template <typename T>
    struct next_has_in_wrapper<volatile T> {
    public:
        enum {
            VALUE = has_in_wrapper<T>::VALUE
        };
    };


    /// Check to see if a derivation of that type has a wrapper.
    template <typename T>
    struct next_has_out_wrapper {
    public:
        enum {
            VALUE = 0
        };
    };


    template <typename T>
    struct next_has_out_wrapper<T *> {
    public:
        enum {
            VALUE = has_out_wrapper<T>::VALUE
        };
    };


    template <typename T>
    struct next_has_out_wrapper<const T> {
    public:
        enum {
            VALUE = has_out_wrapper<T>::VALUE
        };
    };


    template <typename T>
    struct next_has_out_wrapper<volatile T> {
    public:
        enum {
            VALUE = has_out_wrapper<T>::VALUE
        };
    };


    /// Handle some special cases for "dead" types.
#define WRAP_DEAD_TYPE(type) \
    template <> \
    struct has_in_wrapper<type> { \
        enum { \
            VALUE = 0 \
        }; \
    }; \
    template <> \
    struct has_out_wrapper<type> { \
        enum { \
            VALUE = 0 \
        }; \
    };



    WRAP_DEAD_TYPE(void)
    WRAP_DEAD_TYPE(const void)
    WRAP_DEAD_TYPE(volatile void)
    WRAP_DEAD_TYPE(const volatile void)


    /// Specialisation to handle tricky cases.
#define WRAP_BASIC_TYPE_IMPL(basic_type) \
    template <> \
    struct has_in_wrapper<basic_type> { \
    public: \
        enum { \
            VALUE = 0 \
        }; \
    }; \
    template <> \
    struct has_out_wrapper<basic_type> { \
    public: \
        enum { \
            VALUE = 0 \
        }; \
    }; \
    \
    template <> \
    struct type_wrapper<basic_type> { \
    public: \
        enum { \
            HAS_IN_WRAPPER              = 0, \
            HAS_PRE_IN_WRAPPER          = 0, \
            HAS_POST_IN_WRAPPER         = 0, \
            HAS_RETURN_IN_WRAPPER       = 0, \
            \
            HAS_OUT_WRAPPER             = 0, \
            HAS_PRE_OUT_WRAPPER         = 0, \
            HAS_POST_OUT_WRAPPER        = 0, \
            HAS_RETURN_OUT_WRAPPER      = 0, \
            \
            HAS_META_INFO_TRACKER       = 0 \
        }; \
        \
        typedef basic_type T; \
        \
        inline static void pre_in_wrap(T, const int) throw() { } \
        inline static void post_in_wrap(T, const int) throw() { } \
        inline static void return_in_wrap(T, const int) throw() { } \
        \
        inline static void pre_out_wrap(T, const int) throw() { } \
        inline static void post_out_wrap(T, const int) throw() { } \
        inline static void return_out_wrap(T, const int) throw() { } \
    };


#define WRAP_BASIC_TYPE(basic_type) \
    WRAP_BASIC_TYPE_IMPL(basic_type *) \
    WRAP_BASIC_TYPE_IMPL(const basic_type *) \
    WRAP_BASIC_TYPE_IMPL(volatile basic_type *)

    /// Tracks whether the function is already wrapped (by a custom function
    /// wrapper).
    template <enum function_wrapper_id>
    struct is_function_wrapped {
        enum {
            VALUE = 0
        };
    };


    /// Wrapping operators.
    struct pre_in_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_in_wrapper<T>::VALUE | PRE_WRAP_MASK) {
                type_wrapper<T>::pre_in_wrap(val, MAX_PRE_WRAP_DEPTH);
            }
        }

        template <typename T>
        static inline void apply(T &val, const int depth) throw() {
            if(has_in_wrapper<T>::VALUE | PRE_WRAP_MASK) {
                type_wrapper<T>::pre_in_wrap(val, depth);
            }
        }
    };


    struct pre_out_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_out_wrapper<T>::VALUE | PRE_WRAP_MASK) {
                type_wrapper<T>::pre_out_wrap(val, MAX_PRE_WRAP_DEPTH);
            }
        }

        template <typename T>
        static inline void apply(T &val, const int depth) throw() {
            if(has_out_wrapper<T>::VALUE | PRE_WRAP_MASK) {
                type_wrapper<T>::pre_out_wrap(val, depth);
            }
        }
    };


    struct post_in_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_in_wrapper<T>::VALUE | POST_WRAP_MASK) {
                type_wrapper<T>::post_in_wrap(val, MAX_POST_WRAP_DEPTH);
            }
        }

        template <typename T>
        static inline void apply(T &val, const int depth) throw() {
            if(has_in_wrapper<T>::VALUE | POST_WRAP_MASK) {
                type_wrapper<T>::post_in_wrap(val, depth);
            }
        }
    };


    struct post_out_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_out_wrapper<T>::VALUE | POST_WRAP_MASK) {
                type_wrapper<T>::post_out_wrap(val, MAX_POST_WRAP_DEPTH);
            }
        }

        template <typename T>
        static inline void apply(T &val, const int depth) throw() {
            if(has_out_wrapper<T>::VALUE | POST_WRAP_MASK) {
                type_wrapper<T>::post_out_wrap(val, depth);
            }
        }
    };


    struct return_in_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_in_wrapper<T>::VALUE | RETURN_WRAP_MASK) {
                type_wrapper<T>::return_in_wrap(val, MAX_RETURN_WRAP_DEPTH);
            }
        }

        template <typename T>
        static inline void apply(T &val, const int depth) throw() {
            if(has_in_wrapper<T>::VALUE |RETURN_WRAP_MASK) {
                type_wrapper<T>::return_in_wrap(val, depth);
            }
        }
    };


    struct return_out_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_out_wrapper<T>::VALUE | RETURN_WRAP_MASK) {
                type_wrapper<T>::return_out_wrap(val, MAX_RETURN_WRAP_DEPTH);
            }
        }

        template <typename T>
        static inline void apply(T &val, const int depth) throw() {
            if(has_out_wrapper<T>::VALUE |RETURN_WRAP_MASK) {
                type_wrapper<T>::return_out_wrap(val, depth);
            }
        }
    };


    /// Represents a manual function wrapper.
    struct custom_wrapped_function { };


    /// Represents a generic, type-aware function wrapper.
    template <enum function_wrapper_id id, typename R, typename... Args>
    struct wrapped_function;


    template <enum function_wrapper_id id, bool inherit, typename R, typename... Args>
    struct wrapped_function_impl;


    /// Represents a generic, type-aware function wrapper, where the wrapped
    /// function returns a non-void type.
    template <enum function_wrapper_id id, typename R, typename... Args>
    struct wrapped_function_impl<id, false, R, Args...> {
    public:

        typedef R func_type(Args...);


        /// Call the function to be wrapped. Before calling, this will first
        /// pre-wrap the arguments. After the function is called, it will post-
        /// wrap the arguments.
        static R apply(Args... args) throw() {
            P( printf("wrapper(%s)\n", FUNCTION_WRAPPERS[id].name); )

#if GRANARY_IN_KERNEL
            func_type *func((func_type *) kernel_address<id>::VALUE);
#else
            func_type *func(
                (func_type *) FUNCTION_WRAPPERS[id].original_address);
#endif

            apply_to_values<pre_out_wrap, Args...>::apply(args...);
            R ret(func(args...));
            apply_to_values<post_out_wrap, Args...>::apply(args...);
            apply_to_values<return_out_wrap, R>::apply(ret);
            return ret;
        }
    };


    /// Choose the correct custom implementation of this function.
    template <enum function_wrapper_id id, typename R, typename... Args>
    struct wrapped_function_impl<id, true, R, Args...>
        : public wrapped_function_impl<id, false, custom_wrapped_function>
    { };


    /// Represents a generic, type-aware function wrapper, where the wrapped
    /// function returns void.
    template <enum function_wrapper_id id, typename... Args>
    struct wrapped_function_impl<id, false, void, Args...> {
    public:

        typedef void R;
        typedef R func_type(Args...);


        /// Call the function to be wrapped. Before calling, this will first
        /// pre-wrap the arguments. After the function is called, it will post-
        /// wrap the arguments.
        static void apply(Args... args) throw() {
            P( printf("wrapper(%s)\n", FUNCTION_WRAPPERS[id].name); )

#if GRANARY_IN_KERNEL
            func_type *func((func_type *) kernel_address<id>::VALUE);
#else
            func_type *func(
                (func_type *) FUNCTION_WRAPPERS[id].original_address);
#endif

            apply_to_values<pre_out_wrap, Args...>::apply(args...);
            func(args...);
            apply_to_values<post_out_wrap, Args...>::apply(args...);
        }
    };


    /// Represents a "bad" instantiation of the `wrapped_function` template.
    /// If this is instantiated, then it implies that something is missing,
    /// e.g. a wrapper in `granary/gen/*_wrappers.h`.
    template <enum function_wrapper_id id>
    struct wrapped_function_impl<id, false, custom_wrapped_function> {

        // intentionally missing `apply` function so that a reasonable compiler
        // error will give us the function id as well as its type signature.

    };


    /// Choose between a specific wrapped function implementation.
    template <enum function_wrapper_id id, typename R, typename... Args>
    struct wrapped_function : public wrapped_function_impl<
        id,
        is_function_wrapped<id>::VALUE,
        R,
        Args...
    > { };


    /// Return the address of a function wrapper given its id and type.
    template <enum function_wrapper_id id, typename R, typename... Args>
    FORCE_INLINE
    constexpr app_pc wrapper_of(R (*)(Args...)) throw() {
        return reinterpret_cast<app_pc>(wrapped_function<id, R, Args...>::apply);
    }


    /// Return the address of a function wrapper given its id and type, where
    /// the function being wrapped is a C-style variadic function.
    template <enum function_wrapper_id id, typename T>
    FORCE_INLINE
    constexpr app_pc wrapper_of(T *) throw() {
        return reinterpret_cast<app_pc>(
            wrapped_function<id, custom_wrapped_function>::apply);
    }


    /// A pretty ugly hack to pass the target address from gencode into a
    /// generic dynamic wrapper.
#define GET_DYNAMIC_ADDR(var, R, Args) \
    register R (*var)(Args...) asm("r10"); \
    ASM("movq %%r10, %0;" : "=r"(var))


    /// A function that dynamically wraps an instrumented module function and
    /// returns a value.
    template <typename R, typename... Args>
    struct dynamically_wrapped_function {
    public:
        static R apply(Args... args) throw() {
            GET_DYNAMIC_ADDR(func_addr, R, Args);

            P( printf("dynamic_wrapper(%p)\n", (void *) func_addr); )

            apply_to_values<pre_in_wrap, Args...>::apply(args...);
            R ret(func_addr(args...));

#if !CONFIG_ENABLE_DIRECT_RETURN
            granary::detach();
#endif

            apply_to_values<post_in_wrap, Args...>::apply(args...);
            apply_to_values<return_in_wrap, R>::apply(ret);
            return ret;
        }
    };


    /// Temporarily disable the check for uninitalised variables so it doesn't
    /// warn us about `func_addr`, which takes its value from `%r10`.
#   if defined(__clang__)
#       pragma clang diagnostic push
#       pragma clang diagnostic ignored "-Wuninitialized"
#   elif defined(GCC_VERSION) || defined(__GNUC__)
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Wuninitialized"
#   else
#       error "Can't disable compiler warnings around `(user/kernel)_types.h` include."
#   endif


    /// A function that dynamically wraps an instrumented module function and
    /// returns nothing.
    template <typename... Args>
    struct dynamically_wrapped_function<void, Args...> {
    public:
        static void apply(Args... args) throw() {
            GET_DYNAMIC_ADDR(func_addr, void, Args);

            P( printf("dynamic_wrapper(%p)\n", (void *) func_addr); )

            apply_to_values<pre_in_wrap, Args...>::apply(args...);
            func_addr(args...);

#if !CONFIG_ENABLE_DIRECT_RETURN
            detach();
#endif

            apply_to_values<post_in_wrap, Args...>::apply(args...);
        }
    };


#   if defined(__clang__)
#       pragma clang diagnostic pop
#   elif defined(GCC_VERSION) || defined(__GNUC__)
#       pragma GCC diagnostic pop
#   endif


    /// Return the dynamic wrapper address for a wrapper / wrappee.
    /// See granary/dynamic_wrapper.cc
    app_pc dynamic_wrapper_of(app_pc wrapper, app_pc wrappee) throw();


    /// Returns True iff we will will/would dynamic wrap this function.
    template <typename R, typename... Args>
    bool will_dynamic_wrap(R (*app_addr)(Args...)) throw() {
        app_pc app_addr_pc(reinterpret_cast<app_pc>(app_addr));

        if(!is_valid_address(app_addr_pc)) {
            return false;
        }

        // make sure we're not wrapping libc/kernel code, or double-wrapping a
        // wrapper; and make sure that what we're wrapping is sane, i.e. that
        // we're not trying to wrap some garbage memory (it's an app address).
        if(is_host_address(app_addr_pc)
        IF_KERNEL( || is_wrapper_address(app_addr_pc) )
        IF_KERNEL( || !is_app_address(app_addr) )) {
            return false;
        }

        return true;
    }


    /// Return
    template <typename R, typename... Args>
    inline bool is_dynamically_wrapped(R (*app_addr)(Args...)) throw() {
        app_pc app_addr_pc(reinterpret_cast<app_pc>(app_addr));
        return is_wrapper_address(app_addr_pc);
    }


    /// Return the address of a dynamic wrapper for a function.
    template <typename R, typename... Args>
    R (*dynamic_wrapper_of(R (*app_addr)(Args...)))(Args...) throw() {
        typedef R (func_type)(Args...);

        if(!will_dynamic_wrap(app_addr)) {
            return app_addr;
        }

        app_pc app_addr_pc(reinterpret_cast<app_pc>(app_addr));

        IF_KERNEL( ASSERT(!is_code_cache_address(app_addr_pc)); )

        app_pc wrapper_func(unsafe_cast<app_pc>(
            dynamically_wrapped_function<R, Args...>::apply));

        return unsafe_cast<func_type *>(
            dynamic_wrapper_of(wrapper_func, app_addr_pc));
    }


    /// Automatically wrap function pointers.
    template <typename R, typename... Args>
    struct type_wrapper<R (*)(Args...)> {
        enum {
            HAS_IN_WRAPPER              = 1,
            HAS_PRE_IN_WRAPPER          = 1,
            HAS_POST_IN_WRAPPER         = 0,
            HAS_RETURN_IN_WRAPPER       = 0,

            HAS_OUT_WRAPPER             = 1,
            HAS_PRE_OUT_WRAPPER         = 1,
            HAS_POST_OUT_WRAPPER        = 0,
            HAS_RETURN_OUT_WRAPPER      = 0,

            HAS_META_INFO_TRACKER       = 0
        };

        typedef R (*T)(Args...);

        FORCE_INLINE static void pre_in_wrap(T &func_ptr, const int) throw() {
            func_ptr = dynamic_wrapper_of(func_ptr);
        }

        FORCE_INLINE static void post_in_wrap(T &, const int) throw() { }

        FORCE_INLINE static void return_in_wrap(T &, const int) throw() { }

        FORCE_INLINE static void pre_out_wrap(T &func_ptr, const int) throw() {
            func_ptr = dynamic_wrapper_of(func_ptr);
        }

        FORCE_INLINE static void post_out_wrap(T &, const int) throw() { }

        FORCE_INLINE static void return_out_wrap(T &, const int) throw() { }
    };


    /// The identity of a type.
    template <typename T>
    struct identity {
    public:
        typedef T type;
    };


    /// Type without const.
    template <typename T>
    struct constless {
    public:
        typedef T type;
    };


    template <typename T>
    struct constless<const T> {
    public:
        typedef typename constless<T>::type type;
    };


    template <typename T>
    struct constless<T *> {
    public:
        typedef typename constless<T>::type *type;
    };


    template <typename T>
    struct constless<T &> {
    public:
        typedef typename constless<T>::type &type;
    };


    template <typename R, typename... Args>
    struct constless<R (* const)(Args...)> {
    public:
        typedef R (*type)(Args...);
    };


    template <typename T>
    struct referenceless {
    public:
        typedef T type;
    };


    template <typename T>
    struct referenceless<T &> {
    public:
        typedef T type;
    };


    template <typename T>
    struct referenceless<T &&> {
    public:
        typedef T type;
    };
}


#define TYPE_WRAPPER_FUNCTION(prefix) \
    FORCE_INLINE static void \
    CAT(prefix, _wrap)(wrapped_type__ &arg__, int depth__) throw() { \
        if(0 < depth__) { \
            impl__::CAT(prefix, _wrap)(arg__, depth__ - 1); \
        } \
    } \

#define TYPE_WRAPPER(type_name, wrap_code) \
    namespace granary { \
        template <> \
        struct type_wrapper<type_name> { \
            typedef identity<type_name>::type wrapped_type__; \
            typedef type_wrapper_base<type_name> base_wrapper_type__; \
            \
            struct impl__: public type_wrapper_base<type_name> \
            wrap_code; \
            \
            enum { \
                HAS_META_INFO_TRACKER = 0, \
                HAS_PRE_IN_WRAPPER = impl__::HAS_PRE_IN_WRAPPER, \
                HAS_PRE_OUT_WRAPPER = impl__::HAS_PRE_OUT_WRAPPER, \
                HAS_POST_IN_WRAPPER = impl__::HAS_POST_IN_WRAPPER, \
                HAS_POST_OUT_WRAPPER = impl__::HAS_POST_OUT_WRAPPER, \
                HAS_RETURN_IN_WRAPPER = impl__::HAS_RETURN_IN_WRAPPER, \
                HAS_RETURN_OUT_WRAPPER = impl__::HAS_RETURN_OUT_WRAPPER, \
                HAS_IN_WRAPPER = 0 \
                               | (HAS_PRE_IN_WRAPPER << 0) \
                               | (HAS_POST_IN_WRAPPER << 1) \
                               | (HAS_RETURN_IN_WRAPPER << 2), \
                HAS_OUT_WRAPPER = 0 \
                                | (HAS_PRE_OUT_WRAPPER << 0) \
                                | (HAS_POST_OUT_WRAPPER << 1) \
                                | (HAS_RETURN_OUT_WRAPPER << 2) \
            }; \
            \
            TYPE_WRAPPER_FUNCTION(pre_in) \
            TYPE_WRAPPER_FUNCTION(pre_out) \
            TYPE_WRAPPER_FUNCTION(post_in) \
            TYPE_WRAPPER_FUNCTION(post_out) \
            TYPE_WRAPPER_FUNCTION(return_in) \
            TYPE_WRAPPER_FUNCTION(return_out) \
        }; \
    }

    template <typename T>
    struct not_qualified {
        typedef T type;
    };

    template <typename T>
    struct not_qualified<const T> { };

    template <typename T>
    struct not_qualified<volatile T> { };


#define POINTER_WRAPPER_QUAL(qual, wrap_code) \
    namespace granary { \
        template <typename T> \
        struct type_wrapper<qual T *> { \
            typedef qual T *wrapped_type__; \
            typedef type_wrapper_base<qual T *> base_wrapper_type__; \
            \
            struct impl__: public type_wrapper_base<qual T *> \
            wrap_code; \
            \
            enum { \
                HAS_META_INFO_TRACKER = 0, \
                HAS_PRE_IN_WRAPPER = impl__::HAS_PRE_IN_WRAPPER, \
                HAS_PRE_OUT_WRAPPER = impl__::HAS_PRE_OUT_WRAPPER, \
                HAS_POST_IN_WRAPPER = impl__::HAS_POST_IN_WRAPPER, \
                HAS_POST_OUT_WRAPPER = impl__::HAS_POST_OUT_WRAPPER, \
                HAS_RETURN_IN_WRAPPER = impl__::HAS_RETURN_IN_WRAPPER, \
                HAS_RETURN_OUT_WRAPPER = impl__::HAS_RETURN_OUT_WRAPPER, \
                HAS_IN_WRAPPER = 0 \
                               | (HAS_PRE_IN_WRAPPER << 0) \
                               | (HAS_POST_IN_WRAPPER << 1) \
                               | (HAS_RETURN_IN_WRAPPER << 2), \
                HAS_OUT_WRAPPER = 0 \
                                | (HAS_PRE_OUT_WRAPPER << 0) \
                                | (HAS_POST_OUT_WRAPPER << 1) \
                                | (HAS_RETURN_OUT_WRAPPER << 2) \
            }; \
            \
            TYPE_WRAPPER_FUNCTION(pre_in) \
            TYPE_WRAPPER_FUNCTION(pre_out) \
            TYPE_WRAPPER_FUNCTION(post_in) \
            TYPE_WRAPPER_FUNCTION(post_out) \
            TYPE_WRAPPER_FUNCTION(return_in) \
            TYPE_WRAPPER_FUNCTION(return_out) \
        }; \
    }

#define POINTER_WRAPPER(wrap_code) \
    namespace granary { \
        template <typename T> \
        struct type_wrapper<T *> { \
            typedef T *wrapped_type__; \
            typedef type_wrapper_base<T *> base_wrapper_type__; \
            \
            struct impl__: public type_wrapper_base<T *> \
            wrap_code; \
            \
            enum { \
                HAS_META_INFO_TRACKER = 0, \
                HAS_PRE_IN_WRAPPER = impl__::HAS_PRE_IN_WRAPPER, \
                HAS_PRE_OUT_WRAPPER = impl__::HAS_PRE_OUT_WRAPPER, \
                HAS_POST_IN_WRAPPER = impl__::HAS_POST_IN_WRAPPER, \
                HAS_POST_OUT_WRAPPER = impl__::HAS_POST_OUT_WRAPPER, \
                HAS_RETURN_IN_WRAPPER = impl__::HAS_RETURN_IN_WRAPPER, \
                HAS_RETURN_OUT_WRAPPER = impl__::HAS_RETURN_OUT_WRAPPER, \
                HAS_IN_WRAPPER = 0 \
                               | (HAS_PRE_IN_WRAPPER << 0) \
                               | (HAS_POST_IN_WRAPPER << 1) \
                               | (HAS_RETURN_IN_WRAPPER << 2), \
                HAS_OUT_WRAPPER = 0 \
                                | (HAS_PRE_OUT_WRAPPER << 0) \
                                | (HAS_POST_OUT_WRAPPER << 1) \
                                | (HAS_RETURN_OUT_WRAPPER << 2) \
            }; \
            \
            TYPE_WRAPPER_FUNCTION(pre_in) \
            TYPE_WRAPPER_FUNCTION(pre_out) \
            TYPE_WRAPPER_FUNCTION(post_in) \
            TYPE_WRAPPER_FUNCTION(post_out) \
            TYPE_WRAPPER_FUNCTION(return_in) \
            TYPE_WRAPPER_FUNCTION(return_out) \
        }; \
    }

#define TYPEDEF_WRAPPER(type_name, aliased_type_name) \
    namespace granary { \
        template <> \
        struct type_wrapper<type_name> : public type_wrapper<aliased_type_name> { }; \
    }

#define FUNCTION_WRAPPER(function_name, return_type, arg_list, wrapper_code) \
    namespace granary { \
        template <> \
        struct is_function_wrapped< DETACH_ID_ ## function_name > { \
            enum { \
                VALUE = 1 \
            }; \
        }; \
        template <> \
        struct wrapped_function_impl< \
            DETACH_ID_ ## function_name, \
            false, \
            custom_wrapped_function \
        > { \
        public: \
            typedef referenceless<PARAMS return_type>::type R; \
            static R apply arg_list throw() { \
                IF_KERNEL(auto function_name((decltype(::function_name) *) \
                    DETACH_ADDR_ ## function_name );) \
                int depth__(MAX_PRE_WRAP_DEPTH); \
                (void) depth__; \
                wrapper_code \
            } \
        }; \
    }

#define FUNCTION_WRAPPER_VOID(function_name, arg_list, wrapper_code) \
    namespace granary { \
        template <> \
        struct is_function_wrapped< DETACH_ID_ ## function_name > { \
            enum { \
                VALUE = 1 \
            }; \
        }; \
        template <> \
        struct wrapped_function_impl< \
            DETACH_ID_ ## function_name, \
            false, \
            custom_wrapped_function \
        > { \
        public: \
            typedef void R; \
            static void apply arg_list throw() { \
                int depth__(MAX_PRE_WRAP_DEPTH); \
                (void) depth__; \
                wrapper_code \
            } \
        }; \
    }

#define WRAP_FUNCTION(lvalue) \
    { \
        decltype(lvalue) new_lvalue__(dynamic_wrapper_of(lvalue)); \
        if(lvalue != new_lvalue__) { \
            *const_cast<constless<decltype(lvalue)>::type *>(&lvalue) = \
                dynamic_wrapper_of(lvalue); \
        } \
    } (void) depth__

#define ABORT_IF_FUNCTION_IS_WRAPPED(lvalue) \
    { \
        if(IF_USER_ELSE(false, is_dynamically_wrapped(lvalue))) { \
            return; \
        } \
    }


#define ABORT_IF_SUB_FUNCTION_IS_WRAPPED(lvalue, field) \
    { \
        if(0 < depth__ && is_valid_address(lvalue)) { \
            ABORT_IF_FUNCTION_IS_WRAPPED((lvalue)->field); \
        } \
    }

#define PRE_IN_WRAP(lvalue) pre_in_wrap::apply((lvalue), depth__)
#define POST_IN_WRAP(lvalue) post_in_wrap::apply((lvalue), depth__)
#define RETURN_IN_WRAP(lvalue) return_in_wrap::apply((lvalue), depth__)

#define PRE_OUT_WRAP(lvalue) pre_out_wrap::apply((lvalue), depth__)
#define POST_OUT_WRAP(lvalue) post_out_wrap::apply((lvalue), depth__)
#define RETURN_OUT_WRAP(lvalue) return_out_wrap::apply((lvalue), depth__)

#define NO_PRE \
    enum { HAS_PRE_IN_WRAPPER = 0, HAS_PRE_OUT_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(pre_in) \
    EMPTY_WRAP_FUNC_IMPL(pre_out)

#define NO_PRE_IN \
    enum { HAS_PRE_IN_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(pre_in) \

#define NO_PRE_OUT \
    enum { HAS_PRE_OUT_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(pre_out) \

#define NO_POST \
    enum { HAS_POST_IN_WRAPPER = 0, HAS_POST_OUT_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(post_in) \
    EMPTY_WRAP_FUNC_IMPL(post_out)

#define NO_POST_IN \
    enum { HAS_POST_IN_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(post_in) \

#define NO_POST_OUT \
    enum { HAS_POST_OUT_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(post_out) \

#define NO_RETURN \
    enum { HAS_RETURN_IN_WRAPPER = 0, HAS_RETURN_OUT_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(return_in) \
    EMPTY_WRAP_FUNC_IMPL(return_out)

#define NO_RETURN_IN \
    enum { HAS_RETURN_IN_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(return_in) \

#define NO_RETURN_OUT \
    enum { HAS_RETURN_OUT_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(return_out) \

#define PRE_IN \
    enum { HAS_PRE_IN_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(pre_in)

#define PRE_OUT \
    enum { HAS_PRE_OUT_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(pre_out)

#define PRE_INOUT \
    enum { HAS_PRE_IN_WRAPPER = 1, HAS_PRE_OUT_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(pre_out) { pre_in_wrap(arg, depth__); } \
    WRAP_FUNC_IMPL(pre_in)

#define POST_IN \
    enum { HAS_POST_IN_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(post_in)

#define POST_OUT \
    enum { HAS_POST_OUT_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(post_out)

#define POST_INOUT \
    enum { HAS_POST_IN_WRAPPER = 1, HAS_POST_OUT_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(post_out) { post_in_wrap(arg, depth__); } \
    WRAP_FUNC_IMPL(post_in)

#define RETURN_IN \
    enum { HAS_RETURN_IN_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(return_in)

#define RETURN_OUT \
    enum { HAS_RETURN_OUT_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(return_out)

#define RETURN_INOUT \
    enum { HAS_RETURN_IN_WRAPPER = 1, HAS_RETURN_OUT_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(return_out) { return_in_wrap(arg, depth__); } \
    WRAP_FUNC_IMPL(return_in)

#define INHERIT_PRE_IN \
    enum { \
        HAS_PRE_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK) \
    };

#define INHERIT_POST_IN \
    enum { \
        HAS_POST_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK) \
    };

#define INHERIT_RETURN_IN \
    enum { \
        HAS_RETURN_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK) \
    };

#define INHERIT_IN \
    enum { \
        HAS_PRE_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK), \
        HAS_POST_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK), \
        HAS_RETURN_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK) \
    };

#define INHERIT_PRE_OUT \
    enum { \
        HAS_PRE_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK) \
    };

#define INHERIT_POST_OUT \
    enum { \
        HAS_POST_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK) \
    };

#define INHERIT_RETURN_OUT \
    enum { \
        HAS_RETURN_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK) \
    };

#define INHERIT_OUT \
    enum { \
        HAS_PRE_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK), \
        HAS_POST_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK), \
        HAS_RETURN_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK) \
    };

#define INHERIT_POST_INOUT \
    enum { \
        HAS_POST_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK), \
        HAS_POST_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK) \
    };

#define INHERIT_PRE_INOUT \
    enum { \
        HAS_PRE_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK), \
        HAS_PRE_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK) \
    };

#define INHERIT_RETURN_INOUT \
    enum { \
        HAS_RETURN_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK), \
        HAS_RETURN_OUT_WRAPPER = !!(next_has_out_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK) \
    };

#define INHERIT_INOUT \
    enum { \
        HAS_PRE_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK), \
        HAS_POST_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK), \
        HAS_RETURN_IN_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK), \
        HAS_PRE_OUT_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & PRE_WRAP_MASK), \
        HAS_POST_OUT_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & POST_WRAP_MASK), \
        HAS_RETURN_OUT_WRAPPER = !!(next_has_in_wrapper<wrapped_type__>::VALUE & RETURN_WRAP_MASK) \
    };

#define WRAP_FUNC_IMPL(kind) \
    static inline void kind ## _wrap(wrapped_type__ &arg, int depth__) throw()

#define EMPTY_WRAP_FUNC_IMPL(kind) \
    static inline void kind ## _wrap(wrapped_type__ &, int) throw() { }


#endif /* CONFIG_ENABLE_WRAPPERS */
#endif /* granary_WRAPPER_H_ */
