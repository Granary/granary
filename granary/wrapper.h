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

        inline static void pre_wrap(T &, const unsigned /* depth__ */) throw() { }
        inline static void post_wrap(T &, const unsigned /* depth__ */) throw() { }
        inline static void return_wrap(T &, const unsigned /* depth__ */) throw() { }
    };


    /// Represents a generic, unwrapped type.
    template <typename T>
    struct type_wrapper : public type_wrapper_base<T> {
    public:
        enum {

            /// Does this type have any wrappers? This will be a bitmask of
            /// the pre/post/return wrappers, so we can detect which are
            /// present.
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


    /// Specialisation to handle tricky cases.
#define WRAP_BASIC_TYPE_IMPL(basic_type) \
    template <> \
    struct has_wrapper<basic_type> { \
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
            HAS_WRAPPER             = 0, \
            HAS_PRE_WRAPPER         = 0, \
            HAS_POST_WRAPPER        = 0, \
            HAS_RETURN_WRAPPER      = 0, \
            HAS_META_INFO_TRACKER   = 0 \
        }; \
        \
        typedef basic_type T; \
        \
        inline static void pre_wrap(T, const unsigned) throw() { } \
        inline static void post_wrap(T, const unsigned) throw() { } \
        inline static void return_wrap(T, const unsigned) throw() { } \
    };


#define WRAP_BASIC_TYPE(basic_type) \
    WRAP_BASIC_TYPE_IMPL(basic_type *) \
    WRAP_BASIC_TYPE_IMPL(const basic_type *) \
    WRAP_BASIC_TYPE_IMPL(volatile basic_type *) \
    WRAP_BASIC_TYPE_IMPL(volatile const basic_type *) \


    /// Specialisations to handle "following" types.
    template <typename T>
    struct type_wrapper_base<const T> {
    public:

        inline static void pre_wrap(const T &val, const unsigned depth) throw() {
            type_wrapper<T>::pre_wrap(const_cast<T &>(val), depth);
        }

        inline static void post_wrap(const T &val, const unsigned depth) throw() {
            type_wrapper<T>::post_wrap(const_cast<T &>(val), depth);
        }

        inline static void return_wrap(const T &val, const unsigned depth) throw() {
            type_wrapper<T>::return_wrap(const_cast<T &>(val), depth);
        }
    };


    /// Specialisations to handle "following" types.
    template <typename T>
    struct type_wrapper_base<volatile T> {
    public:

        inline static void pre_wrap(volatile T &val, const unsigned depth) throw() {
            type_wrapper<T>::pre_wrap(reinterpret_cast<T &>(val), depth);
        }

        inline static void post_wrap(volatile T &val, const unsigned depth) throw() {
            type_wrapper<T>::post_wrap(reinterpret_cast<T &>(val), depth);
        }

        inline static void return_wrap(volatile T &val, const unsigned depth) throw() {
            type_wrapper<T>::return_wrap(reinterpret_cast<T &>(val), depth);
        }
    };


    template <typename T>
    struct type_wrapper_base<T *> {
    public:

        inline static void pre_wrap(T *&val, const unsigned depth) throw() {
            if(val) {
                type_wrapper<T>::pre_wrap(*val, depth);
            }
        }

        inline static void post_wrap(T *&val, const unsigned depth) throw() {
            if(val) {
                type_wrapper<T>::post_wrap(*val, depth);
            }
        }

        inline static void return_wrap(T *&val, const unsigned depth) throw() {
            if(val) {
                type_wrapper<T>::return_wrap(*val, depth);
            }
        }
    };


    /// Check to see if something, or some derivation of that thing, has a
    /// wrapper.
    template <typename T>
    struct has_wrapper {
    public:
        enum {
            VALUE = type_wrapper<T>::HAS_WRAPPER
        };
    };


    template <typename T>
    struct has_wrapper<T *> {
    public:
        enum {
            VALUE = has_wrapper<T>::VALUE | type_wrapper<T *>::HAS_WRAPPER
        };
    };


    template <typename T>
    struct has_wrapper<const T> {
    public:
        enum {
            VALUE = has_wrapper<T>::VALUE | type_wrapper<const T>::HAS_WRAPPER
        };
    };


    WRAP_BASIC_TYPE(void)
    WRAP_BASIC_TYPE(char)
    WRAP_BASIC_TYPE(unsigned char)
    WRAP_BASIC_TYPE(short)
    WRAP_BASIC_TYPE(unsigned short)
    WRAP_BASIC_TYPE(int)
    WRAP_BASIC_TYPE(unsigned)
    WRAP_BASIC_TYPE(long)
    WRAP_BASIC_TYPE(unsigned long)
    WRAP_BASIC_TYPE(long long)
    WRAP_BASIC_TYPE(unsigned long long)
    WRAP_BASIC_TYPE(float)
    WRAP_BASIC_TYPE(double)


    /// Tracks whether the function is already wrapped (by a custom function
    /// wrapper).
    template <enum function_wrapper_id>
    struct is_function_wrapped {
        enum {
            VALUE = 0
        };
    };


    /// Wrapping operators.
    struct pre_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_wrapper<T>::VALUE | PRE_WRAP_MASK) {
                type_wrapper<T>::pre_wrap(val, MAX_PRE_WRAP_DEPTH);
            }
        }
    };


    struct post_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_wrapper<T>::VALUE | POST_WRAP_MASK) {
                type_wrapper<T>::post_wrap(val, MAX_POST_WRAP_DEPTH);
            }
        }
    };


    struct return_wrap {
    public:
        template <typename T>
        static inline void apply(T &val) throw() {
            if(has_wrapper<T>::VALUE | RETURN_WRAP_MASK) {
                type_wrapper<T>::return_wrap(val, MAX_RETURN_WRAP_DEPTH);
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

            func_type *func(
                (func_type *) FUNCTION_WRAPPERS[id].original_address);

            apply_to_values<pre_wrap, Args...>::apply(args...);
            R ret(func(args...));
            apply_to_values<post_wrap, Args...>::apply(args...);
            apply_to_values<return_wrap, R>::apply(ret);
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

            func_type *func(
                (func_type *) FUNCTION_WRAPPERS[id].original_address);

            apply_to_values<pre_wrap, Args...>::apply(args...);
            func(args...);
            apply_to_values<post_wrap, Args...>::apply(args...);
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

            apply_to_values<pre_wrap, Args...>::apply(args...);
            R ret(func_addr(args...));
            granary::detach();

            apply_to_values<post_wrap, Args...>::apply(args...);
            apply_to_values<return_wrap, R>::apply(ret);
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

            apply_to_values<pre_wrap, Args...>::apply(args...);
            func_addr(args...);
            detach();
            apply_to_values<post_wrap, Args...>::apply(args...);
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

        if(!app_addr_pc) {
            return app_addr;
        }

        // make sure we're not wrapping libc/kernel code, or double-wrapping a
        // wrapper; and make sure that what we're wrapping is sane, i.e. that
        // we're not trying to wrap some garbage memory (it's an app address).
        if(is_host_address(app_addr)
        || is_wrapper_address(app_addr_pc)
        || !is_app_address(app_addr)) {
            return unsafe_cast<func_type *>(app_addr);
        }

        ASSERT(!is_code_cache_address(app_addr_pc));

        app_pc wrapper_func(unsafe_cast<app_pc>(
            dynamically_wrapped_function<R, Args...>::apply));

        return unsafe_cast<func_type *>(
            dynamic_wrapper_of(wrapper_func, app_addr_pc));
    }


    /// Automatically wrap function pointers.
    template <typename R, typename... Args>
    struct type_wrapper<R (*)(Args...)> {
        enum {
            HAS_WRAPPER             = 1,
            HAS_PRE_WRAPPER         = 1,
            HAS_POST_WRAPPER        = 0,
            HAS_RETURN_WRAPPER      = 0,
            HAS_META_INFO_TRACKER   = 0
        };

        typedef R (*T)(Args...);

        inline static void pre_wrap(T &func_ptr, const unsigned) throw() {
            func_ptr = dynamic_wrapper_of(func_ptr);
        }

        inline static void post_wrap(T &, const unsigned) throw() { }

        inline static void return_wrap(T &, const unsigned) throw() { }
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
                HAS_META_INFO_TRACKER = 0, \
                HAS_PRE_WRAPPER = impl__::HAS_PRE_WRAPPER, \
                HAS_POST_WRAPPER = impl__::HAS_POST_WRAPPER, \
                HAS_RETURN_WRAPPER = impl__::HAS_RETURN_WRAPPER, \
                HAS_WRAPPER = HAS_PRE_WRAPPER \
                            | (HAS_POST_WRAPPER << 1) \
                            | (HAS_RETURN_WRAPPER << 1) \
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
