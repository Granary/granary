/*
 * rcu.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_RCU_H_
#define granary_RCU_H_

#include <atomic>

#include "granary/pp.h"
#include "granary/utils.h"

namespace granary { namespace rcu {


    /// Forward declarations.
    template <typename> struct rcu_protected;
    template <typename> struct rcu_read_reference;
    template <typename> struct rcu_write_reference;


    /// Promote a read reference to a write reference. This is only
    /// safe within a read-critical section, and should only be used
    /// when one is changing the "skeleton" of a data structure, and
    /// not the "data" stored in the data structure. It is up to the
    /// programmer to ensure that uses of the returned write
    /// references are correctly used.
    template <typename T>
    rcu_write_reference<T> rcu_promote_reference(rcu_read_reference<T> &) throw();


    /// Get the raw reference to the data used in an RCU-protected
    /// data structure. This is useful when a custom deallocation
    /// strategy is needed for the protected data structure.
    template <typename T>
    T *&rcu_unprotect(rcu_protected<T> &) throw();


    /// Assign a pointer to an RCU read/write-referenced pointer.
    template <typename T>
    void rcu_assign_pointer(T *&, rcu_read_reference<T>) throw();

    template <typename T>
    void rcu_assign_pointer(T *&, rcu_write_reference<T>) throw();


    /// Represents a read reference of an element in an RCU-protected
    /// data structure. A read reference is only safe to used within the
    /// context of its read-critical section.
    template <typename T>
    struct rcu_read_reference {
    private:

        friend struct rcu_protected<T>;

        template <typename R>
        friend rcu_write_reference<R> &rcu_promote_reference(rcu_read_reference<R> &);

        template <typename R>
        friend void rcu_assign_pointer(R *&, rcu_read_reference<R>) throw();

        typedef rcu_read_reference<T> self_type;

        /// Internal pointer to the structure being read.
        const T *original;

        rcu_read_reference(const T *original_) throw()
            : original(original_)
        { }

        rcu_read_reference(void) = delete;
        rcu_read_reference(const self_type &) throw() = delete;
        self_type &operator=(const self_type &) throw() = delete;

    public:

        /// Access a field of the referenced data structure.
        inline const T *operator->(void) const throw() {
            return original;
        }

        inline operator const T *(void) const throw() {
            return original;
        }
    };


    /// Represents a write reference to an RCU-protected data structure.
    /// Write references allow all of the same operations as normal
    /// references, with the exception that they require one to explicitly
    /// recognise that a reference is indeed RCU-related.
    template <typename T>
    struct rcu_write_reference {
    private:

        friend struct rcu_protected<T>;

        template <typename R>
        friend void rcu_assign_pointer(R *&, rcu_write_reference<R>) throw();

        /// Write reference to the data structure; allows easy publishing
        /// of a new version of the referenced structure.
        T **pointer;

        inline rcu_write_reference(T *&pointer_)
            : pointer(&pointer_)
        { }

        rcu_write_reference(void) throw() = delete;

    public:

        rcu_write_reference(const rcu_write_reference<T> &) throw() = default;

        /// Publish a new version of the data structure. This
        /// returns a pointer to the old version.
        T *publish(T *new_version) throw() {
            std::atomic<unsigned> barrier;
            barrier.store(1, std::memory_order_seq_cst);
            T *old_version(*pointer);
            *pointer = new_version;
            barrier.store(1, std::memory_order_seq_cst);
            return old_version;
        }

        inline bool is_valid(void) const throw() {
            return !!*pointer;
        }

        inline T *operator->(void) throw() {
            return *pointer;
        }

        inline const T *operator->(void) const throw() {
            return *pointer;
        }
    };


    /// Represents the protocol of an RCU writer thread.
    template <typename T>
    struct rcu_writer {
    public:
        virtual ~rcu_writer(void) throw() { }

        /// This function is executed before mutual exclusion is acquired over the
        /// data structure.
        virtual void setup(const rcu_write_reference<T>) throw() { }

        /// This function is called after mutual exclusion has been acquired by the
        /// writer (and so this the only writer writing to the data structure), but
        /// before we have waited for all readers to complete their read-critical
        /// sections.
        virtual void while_readers_exist(rcu_write_reference<T>) throw() { }

        /// This function is called after all readers are done completing their
        /// read-critical sections, but before the writer releases its lock on the
        /// data structure.
        virtual void after_readers_done(rcu_write_reference<T>) throw() { }

        /// This function is called after the writer releases its lock on the
        /// data structure.
        virtual void teardown(rcu_write_reference<T>) throw() { }
    };


    /// Tag to specify that a data structure should not be allocated by default.
    struct dont_initialize { };


    /// Represents an RCU-protected data structure. This encompasses simple uses
    /// of the RCU API.
    template <typename T>
    struct rcu_protected {
    private:

        template <typename R>
        friend R *&rcu_unprotect(rcu_protected<R> &) throw();

        typedef rcu_read_reference<T> read_reference;

        /// The data structure being protected.
        T *value;

        /// A pointer to the reference counter of the currently active generation.
        /// When we do a write, this pointer is changed, so the writer can wait on
        /// the old value (thus waiting out the readers that were active at the
        /// time of the write) and future readers will "queue" up for their writer
        /// using the new reference counter.
        std::atomic<reference_counter *> active_read_counter;

        /// A writer lock on the entire data structure.
        spin_lock write_lock;

    public:

        rcu_protected(rcu_protected<T> &) throw() = delete;

        rcu_protected(void) throw()
            : value(new T)
            , active_read_counter(new reference_counter)
        { }

        rcu_protected(dont_initialize) throw()
            : value(nullptr)
            , active_read_counter(new reference_counter)
        { }

        template <typename... Args>
        rcu_protected(Args... args) throw()
            : value(new T(args...))
            , active_read_counter(new reference_counter)
        { }

        ~rcu_protected(void) throw() {
            if(value) {
                delete value;
                value = nullptr;
            }

            delete active_read_counter.load();
            active_read_counter.store(nullptr);
        }

        /// Enter a read-critical section., where the function invoked for
        /// the read-critical section returns a non-void value.
        template <typename R, typename... Args>
        typename std::enable_if<!std::is_void<R>::value, R>::type
        read(
            R (*critical_section)(read_reference &, Args&...),
            Args&... args
        ) const throw() {
            reference_counter *read_counter(nullptr);
            do {
                read_counter = active_read_counter.load(std::memory_order_seq_cst);
            } while(!read_counter->increment());

            read_reference read_ref(value);
            R ret(critical_section(read_ref, args...));
            read_counter->decrement();
            return ret;
        }

        /// Enter a read-critical section, where the function
        /// invoked in the critical section does not return any value.
        template <typename... Args>
        void read(
            void (*critical_section)(read_reference &, Args&...),
            Args&... args
        ) const throw() {
            reference_counter *read_counter(nullptr);
            do {
                read_counter = active_read_counter.load(std::memory_order_seq_cst);
            } while(!read_counter->increment());

            read_reference read_ref(value);
            critical_section(read_ref, args...);
            read_counter->decrement();
        }


        /// Enter a write-critical section. We manage waiting on currently active
        /// readers by changing the reference counter out from under them, so that
        /// new readers entering after the call to setup (which might have changed
        /// the skeleton/structure of the protected data structure) will see a new
        /// reference counter.
        void write(rcu_writer<T> &writer) throw() {
            rcu_write_reference<T> ref(value);
            reference_counter *new_readers(new reference_counter);
            writer.setup(ref);

            write_lock.acquire();
            reference_counter *current_readers(
                active_read_counter.exchange(new_readers, std::memory_order_seq_cst));
            writer.while_readers_exist(ref);
            current_readers->wait();
            writer.after_readers_done(ref);
            write_lock.release();

            writer.teardown(ref);
            delete current_readers;
        }
    };


    /// Explicitly promote a read reference to a write reference.
    template <typename T>
    rcu_write_reference<T>
    rcu_promote_reference(rcu_read_reference<T> &ref) throw() {
        return rcu_write_reference<T>(const_cast<T *>(ref.original));
    }


    /// Explicitly gain access the the raw pointer protected by an
    /// rcu_protected data structure.
    template <typename T>
    T *&rcu_unprotect(rcu_protected<T> &prot) throw() {
        return prot.value;
    }


    /// Assign a pointer given a read reference.
    template <typename T>
    void rcu_assign_pointer(T *&ptr, rcu_read_reference<T> ref) throw() {
        ptr = ref.original;
    }


    /// Assign a pointer given a write reference.
    template <typename T>
    void rcu_assign_pointer(T *&ptr, rcu_write_reference<T> ref) throw() {
        ptr = *(ref.pointer);
    }


    /// Assign a pointer to another one.
    template <typename T>
    void rcu_assign_pointer(T *&ptr, T *new_ptr) throw() {
        std::atomic<unsigned> barrier;
        barrier.store(0, std::memory_order_seq_cst);
        ptr = new_ptr;
        barrier.store(1, std::memory_order_seq_cst);
    }
}}

#endif /* granary_RCU_H_ */
