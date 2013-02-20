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


    /// Represents a manual function wrapper.
    struct custom_wrapped_function { };


    /// Represents a generic, type-aware function wrapper.
    template <enum function_wrapper_id id, typename R, typename... Args>
    struct wrapped_function;


    /// Represents a generic, type-aware function wrapper, where the wrapped
    /// function returns a non-void type.
    template <enum function_wrapper_id id, typename R, typename... Args>
    struct wrapped_function {
    public:

        /// Distinguish default wrappers from function wrappers
        enum {
            IS_DEFAULT_WRAPPER = 1
        };

        typedef R func_type(Args...);


        /// The function that this wrapper wraps.
        static func_type *WRAPPED_ADDRESS;


        /// Call the function to be wrapped. Before calling, this will first
        /// pre-wrap the arguments. After the function is called, it will post-
        /// wrap the arguments.
        static R apply(Args... args) throw() {
            printf("wrapper(%s)\n", FUNCTION_WRAPPERS[id].name);
            return WRAPPED_ADDRESS(args...);
        }
    };


    /// Static-initialise the wrapper address.
    template <enum function_wrapper_id id, typename R, typename... Args>
    typename wrapped_function<id, R, Args...>::func_type *
    wrapped_function<id, R, Args...>::WRAPPED_ADDRESS = \
    (typename wrapped_function<id, R, Args...>::func_type *) \
    FUNCTION_WRAPPERS[id].original_address;


    /// Represents a generic, type-aware function wrapper, where the wrapped
    /// function returns void.
    template <enum function_wrapper_id id, typename... Args>
    struct wrapped_function<id, void, Args...> {
    public:

        /// Distinguish default wrappers from function wrappers
        enum {
            IS_DEFAULT_WRAPPER = 1
        };

        typedef void R;
        typedef void func_type(Args...);


        /// The function that this wrapper wraps.
        static func_type *WRAPPED_ADDRESS;


        /// Call the function to be wrapped. Before calling, this will first
        /// pre-wrap the arguments. After the function is called, it will post-
        /// wrap the arguments.
        static void apply(Args... args) throw() {
            printf("wrapper(%s)\n", FUNCTION_WRAPPERS[id].name);
            WRAPPED_ADDRESS(args...);
        }
    };


    /// Represents a "bad" instantiation of the `wrapped_function` template.
    /// If this is instantiated, then it implies that something is missing,
    /// e.g. a wrapper in `granary/gen/*_wrappers.h`.
    template <enum function_wrapper_id id>
    struct wrapped_function<id, custom_wrapped_function> {

        // intentionally missing `apply` function so that a reasonable compiler
        // error will give us the function id as well as its type signature.

    };


    /// Static-initialise the wrapper address.
    template <enum function_wrapper_id id, typename... Args>
    typename wrapped_function<id, void, Args...>::func_type *
    wrapped_function<id, void, Args...>::WRAPPED_ADDRESS = \
    (typename wrapped_function<id, R, Args...>::func_type *) \
    FUNCTION_WRAPPERS[id].original_address;


    /// Return the address of a function wrapper given its id and type.
    template <enum function_wrapper_id id, typename R, typename... Args>
    inline constexpr app_pc wrapper_of(R (*)(Args...)) throw() {
        return reinterpret_cast<app_pc>(wrapped_function<id, R, Args...>::apply);
    }


    /// Return the address of a function wrapper given its id and type, where
    /// the function being wrapped is a C-style variadic function.
    template <enum function_wrapper_id id, typename T>
    inline constexpr app_pc wrapper_of(T *) throw() {
        return reinterpret_cast<app_pc>(
            wrapped_function<id, custom_wrapped_function>::apply);
    }
}

#define TYPE_WRAPPER(type_name, wrap_code) \
    namespace granary { \
        template <> \
        struct type_wrapper<type_name> { \
            typedef decltype(*(new type_name)) wrapped_type__; \
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
        struct wrapped_function< \
            DETACH_ID_ ## function_name, \
            custom_wrapped_function \
        > { \
        public: \
            typedef std::remove_reference<PARAMS return_type>::type R; \
            static R apply arg_list throw() { \
                const unsigned depth__(MAX_PRE_WRAP_DEPTH); \
                (void) depth__; \
                wrapper_code \
            } \
        }; \
    }

#define FUNCTION_WRAPPER_VOID(function_name, arg_list, wrapper_code) \
    namespace granary { \
        template <> \
        struct wrapped_function< \
            DETACH_ID_ ## function_name, \
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

#define WRAP_FUNCTION(lvalue) (void) (lvalue); (void) depth__
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
