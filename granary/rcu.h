/*
 * rcu.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_RCU_H_
#define granary_RCU_H_

#include "granary/pp.h"
#include "granary/utils.h"

namespace granary {


/// Used to define the classical RCU protocol on a generic data structure.
#define RCU_GENERIC_DATA_STRUCTURE(types, type, ...) \
    union rcu_protected<type TEMPLATE_PARAMS types> { \
    public: \
        typedef type TEMPLATE_PARAMS types base_type__; \
        rcu_private<base_type__> value__; \
        __VA_ARGS__ \
    } __attribute__((packed));


/// Used to define the Classical RCU protocol on a data structure.
#define RCU_DATA_STRUCTURE(type, ...) \
    template <> \
    RCU_GENERIC_DATA_STRUCTURE((), type, ##__VA_ARGS__)


/// Used to define embedded references and values within an RCU-protected
/// data structure.
#define RCU_REFERENCE_FIELD_IMPL(field_name, ref_kind) \
    struct { \
        typedef decltype((new base_type__)->field_name) TYPEOF_ ## field_name; \
        typedef rcu_ ## ref_kind<TYPEOF_ ## field_name> REF_TYPEOF_ ## field_name; \
        inline operator REF_TYPEOF_ ## field_name (void) throw() { \
            return REF_TYPEOF_ ## field_name( \
                unsafe_cast<base_type__ *>(this)->field_name); \
        } \
    } field_name; \


/// Used to define an embedded reference field within an RCU-protected data
/// structure element.
#define RCU_REFERENCE_FIELD(field_name) \
    RCU_REFERENCE_FIELD_IMPL(field_name, reference)


/// Used to define a value within an element within an RCU-protected data
/// structure.
#define RCU_VALUE_FIELD(field_name) \
    RCU_REFERENCE_FIELD_IMPL(field_name, value)


    /// Forward declarations.
    template <typename T> struct rcu_private;
    template <typename T> struct rcu_protected;
    template <typename T> struct rcu_reference;
    template <typename T> struct rcu_value;
    template <typename T> struct rcu_read_reference;
    template <typename T> struct rcu_write_reference;

    template <typename T>
    struct foo {
        foo *next;
        T bar;
    };


    template <typename T>
    RCU_GENERIC_DATA_STRUCTURE((T), foo,
        RCU_REFERENCE_FIELD(next)
        RCU_VALUE_FIELD(bar));

    /// Represents the internal value stored in an rcu protected structure.
    /// This value is unreadable in the usual sense, except by the structure
    /// itself.
    template <typename T>
    struct rcu_private {
    private:
        friend struct rcu_protected<T>;

        T value;
    };


    /// Represents a reference to a field within an RCU-protected data
    /// structure.
    template <typename T>
    struct rcu_reference {
    private:

        T &reference;

    public:

        inline rcu_reference(T &reference_) throw()
            : reference(reference_)
        { }
    };


    /// Represents a read reference.
    template <typename T>
    struct rcu_read_reference {
    protected:
        const T *val;

    public:

    };
}


#endif /* granary_RCU_H_ */
