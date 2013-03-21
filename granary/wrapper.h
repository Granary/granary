/*
 * wrapper.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
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


    /// Base of a generic, unwrapped type. This exists to implement "null"
    /// functionality so that not all type wrappers need to specify pre/post/
    /// return wrappers.
    template <typename T>
    struct type_wrapper_base {
    public:

        inline static void pre_wrap(T &, const unsigned /* depth__ */) throw() { }
        inline static void post_wrap(T &, const unsigned /* depth__ */) throw() { }
        inline static void return_wrap(T &, const unsigned /* depth__ */) throw() { }
    };


    /// Represents a generic, unwrapped type.
    template <typename T>
    struct type_wrapper : public type_wrapper_base<T> {
    public:
        enum {

            /// Does this type have any wrappers?
            HAS_WRAPPER             = 0,

            /// Is there a pre wrapper defined for T? Pre wrappers apply
            /// before a function call.
            HAS_PRE_WRAPPER         = 0,

            /// Is there a post wrapper defined for T? Post wrappers apply
            /// after a function call.
            HAS_POST_WRAPPER        = 0,

            /// Is there a return wrapper defined for T? Return wrappers apply
            /// after a function call on the return value of the function.
            HAS_RETURN_WRAPPER      = 0,

            /// Is a function pointer within T being used to track things (such
            /// as preventing re-wrapping)?
            HAS_META_INFO_TRACKER   = 0
        };
    };


    /// Tracks whether the function is already wrapped (by a custom function
    /// wrapper).
    template <enum function_wrapper_id>
    struct is_function_wrapped {
        enum {
            VALUE = 0
        };
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
            func_type *func(
                (func_type *) FUNCTION_WRAPPERS[id].original_address);
            return func(args...);
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
            func_type *func(
                (func_type *) FUNCTION_WRAPPERS[id].original_address);
            func(args...);
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
    __attribute__((always_inline))
    inline constexpr app_pc wrapper_of(R (*)(Args...)) throw() {
        return reinterpret_cast<app_pc>(wrapped_function<id, R, Args...>::apply);
    }


    /// Return the address of a function wrapper given its id and type, where
    /// the function being wrapped is a C-style variadic function.
    template <enum function_wrapper_id id, typename T>
    __attribute__((always_inline))
    inline constexpr app_pc wrapper_of(T *) throw() {
        return reinterpret_cast<app_pc>(
            wrapped_function<id, custom_wrapped_function>::apply);
    }


    /// A function that dynamically wraps an instrumented module function and
    /// returns a value.
    template <typename R, typename... Args>
    struct dynamically_wrapped_function {
    public:
        static R apply(Args... args) throw() {

            // such an ugly hack :-(
            register R (*func_addr)(Args...) asm("r10");
            ASM("movq %%r10, %0;" : "=r"(func_addr));

            R ret(func_addr(args...));
            granary::detach();
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
            // such an ugly hack :-(
            register void (*func_addr)(Args...) asm("r10");
            ASM("movq %%r10, %0;" : "=r"(func_addr));

            printf("in dynamic wrapper!\n");
            func_addr(args...);
            detach();
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


    /// Return the address of a dynamic wrapper for a function.
    template <typename R, typename... Args>
    R (*dynamic_wrapper_of(R (*app_addr)(Args...)))(Args...) throw() {
        typedef R (func_type)(Args...);

        app_pc app_addr_pc(reinterpret_cast<app_pc>(app_addr));

        // make sure we're not wrapping libc/kernel code, or double-wrapping a
        // wrapper.
        if(is_host_address(app_addr) || is_wrapper_address(app_addr_pc)) {
            return unsafe_cast<func_type *>(app_addr);
        }

        ASSERT(!is_code_cache_address(app_addr_pc));

        app_pc wrapper_func(unsafe_cast<app_pc>(
            dynamically_wrapped_function<R, Args...>::apply));

        return unsafe_cast<func_type *>(
            dynamic_wrapper_of(wrapper_func, app_addr_pc));
    }


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
}

#define TYPE_WRAPPER(type_name, wrap_code) \
    namespace granary { \
        template <> \
        struct type_wrapper<type_name> { \
            typedef identity<type_name>::type wrapped_type__; \
            \
            struct impl__: public type_wrapper_base<type_name> \
            wrap_code; \
            \
            enum { \
                HAS_WRAPPER = 1, \
                HAS_META_INFO_TRACKER = 0, \
                HAS_PRE_WRAPPER = impl__::HAS_PRE_WRAPPER, \
                HAS_POST_WRAPPER = impl__::HAS_POST_WRAPPER, \
                HAS_RETURN_WRAPPER = impl__::HAS_RETURN_WRAPPER \
            }; \
            \
            inline static void \
            pre_wrap(wrapped_type__ &arg__, const unsigned depth__) throw() { \
                if(depth__) { \
                    impl__::pre_wrap(arg__, depth__ - 1); \
                } \
            } \
            \
            inline static void \
            post_wrap(wrapped_type__ &arg__, const unsigned depth__) throw() { \
                if(depth__) { \
                    impl__::post_wrap(arg__, depth__ - 1); \
                } \
            } \
            \
            inline static void \
            return_wrap(wrapped_type__ &arg__, const unsigned depth__) throw() { \
                if(depth__) { \
                    impl__::return_wrap(arg__, depth__ - 1); \
                } \
            } \
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
            typedef std::remove_reference<PARAMS return_type>::type R; \
            static R apply arg_list throw() { \
                IF_KERNEL(auto function_name((decltype(::function_name) *) \
                    DETACH_ADDR_ ## function_name );) \
                const unsigned depth__(MAX_PRE_WRAP_DEPTH); \
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
                const unsigned depth__(MAX_PRE_WRAP_DEPTH); \
                (void) depth__; \
                wrapper_code \
            } \
        }; \
    }

#define WRAP_FUNCTION(lvalue) \
    { \
        *const_cast<constless<decltype(lvalue)>::type *>(&lvalue) = \
            dynamic_wrapper_of(lvalue); \
    } (void) depth__

#define PRE_WRAP(lvalue) (void) (lvalue); (void) depth__
#define POST_WRAP(lvalue) (void) (lvalue); (void) depth__
#define RETURN_WRAP(lvalue) (void) depth__

#define NO_PRE \
    enum { HAS_PRE_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(pre)

#define NO_POST \
    enum { HAS_POST_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(post)

#define NO_RETURN \
    enum { HAS_RETURN_WRAPPER = 0 }; \
    EMPTY_WRAP_FUNC_IMPL(return)

#define PRE \
    enum { HAS_PRE_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(pre)

#define POST \
    enum { HAS_POST_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(post)

#define RETURN \
    enum { HAS_RETURN_WRAPPER = 1 }; \
    WRAP_FUNC_IMPL(return)

#define WRAP_FUNC_IMPL(kind) \
    static inline void kind ## _wrap(wrapped_type__ &arg, unsigned depth__) throw()

#define EMPTY_WRAP_FUNC_IMPL(kind) \
    static inline void kind ## _wrap(wrapped_type__ &, unsigned ) throw() { }

#endif /* CONFIG_ENABLE_WRAPPERS */
#endif /* granary_WRAPPER_H_ */
