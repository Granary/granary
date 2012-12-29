/*
 * wrapper.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_WRAPPER_H_
#define granary_WRAPPER_H_

#include "granary/detach.h"

namespace granary {


    /// Base of a generic, unwrapped type. This exists to implement "null"
    /// functionality so that not all type wrappers need to specify pre/post/
    /// return wrappers.
    template <typename T>
    struct type_wrapper_base {
    public:

        inline void pre_wrap(T &, const unsigned /* depth__ */) throw() { }
        inline void post_wrap(T &, const unsigned /* depth__ */) throw() { }
        inline void return_wrap(T &, const unsigned /* depth__ */) throw() { }
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


    /// Apply a function to each type of an argument pack.
    //template <typename


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
            return WRAPPED_ADDRESS(args...);
        }
    };


    /// Static-initialize the wrapper address.
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
            WRAPPED_ADDRESS(args...);
        }
    };


    /// Static-initialize the wrapper address.
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
        return nullptr; // TODO
    }

}


#endif /* granary_WRAPPER_H_ */
