/*
 * rcu.h
 *
 *  Created on: 2013-01-16
 *      Author: pag
 */

#ifndef GRANARY_RCU_H_
#define GRANARY_RCU_H_


#include "granary/globals.h"
#include "granary/atomic.h"
#include "granary/smp/hazard.h"
#include "granary/smp/spin_lock.h"

namespace granary { namespace smp {

    /// Forward declarations.
    template <typename> struct rcu_protected;
    template <typename> struct rcu_publisher;
    template <typename> struct rcu_collector;
    template <typename> union rcu_read_reference;
    template <typename> union rcu_write_reference;


/// Defines the type protocol for an RCU-protected data structure.
#define RCU_PROTOCOL(type_name, ...) \
    RCU_GENERIC_PROTOCOL((), type_name, (), #__VA_ARGS__)


/// Defines the type protocol for a generic (i.e. one using templates)
/// RCU-protected data structure.
#define RCU_GENERIC_PROTOCOL(tpl_args, type_name, tpl_params, ...) \
    template <PARAMS tpl_args> \
    union rcu_read_reference<type_name TEMPLATE_PARAMS tpl_params > { \
    private: \
        \
        typedef type_name TEMPLATE_PARAMS tpl_params base_type__; \
        friend struct rcu_protected<base_type__>; \
        typedef rcu_read_reference<base_type__> base_ref_type__; \
        typedef rcu_secret::read_secret_type secret_type__; \
        \
        base_type__ *internal_pointer__; \
        \
        inline rcu_read_reference(base_type__ *ptr__) throw() \
            : internal_pointer__(ptr__) \
        { } \
        \
    public: \
        enum { \
            IS_RCU_READ_REFERENCE = 1 \
        }; \
        \
        inline base_type__ *get_reference(secret_type__) throw() { \
            return this->internal_pointer__; \
        } \
        \
        template <typename A__>  \
        inline rcu_read_reference(A__ a__) throw() \
            : internal_pointer__(a__.get_reference(secret_type__())) \
        { } \
        \
        EACH(READ_, NOTHING, NOTHING, __VA_ARGS__) \
        \
        template <typename A__> \
        inline base_ref_type__ &operator=(A__ a__) throw() { \
            static_assert(A__::IS_RCU_READ_REFERENCE, \
                "Argument to operator= must be an RCU read reference type."); \
            this->internal_pointer__ = a__.get_reference(secret_type__()); \
            return *this; \
        } \
        \
        inline operator bool (void) const throw() { \
            return nullptr != (this->internal_pointer__); \
        } \
    }; \
    \
    template <PARAMS tpl_args> \
    union rcu_write_reference<type_name TEMPLATE_PARAMS tpl_params > { \
    private: \
        \
        typedef type_name TEMPLATE_PARAMS tpl_params base_type__; \
        friend struct rcu_protected<base_type__>; \
        template <typename> friend struct rcu_publisher; \
        template <typename> friend struct rcu_collector; \
        typedef rcu_write_reference<base_type__> base_ref_type__; \
        typedef rcu_secret::write_secret_type secret_type__; \
        \
        base_type__ *internal_pointer__; \
        \
        inline rcu_write_reference(base_type__ *ptr__) throw() \
            : internal_pointer__(ptr__) \
        { } \
        \
        inline rcu_write_reference(std::nullptr_t) throw() \
            : internal_pointer__(nullptr) \
        { } \
        \
    public: \
        enum { \
            IS_RCU_WRITE_REFERENCE = 1 \
        }; \
        \
        inline base_type__ *get_reference(secret_type__) throw() { \
            return this->internal_pointer__; \
        } \
        \
        template <typename A__> \
        inline rcu_write_reference(A__ a__) throw() \
            : internal_pointer__(a__.get_reference(secret_type__())) \
        { } \
        \
        rcu_write_reference(void) throw() \
            : internal_pointer__(nullptr) \
        { } \
        \
        template <typename A__> \
        inline base_ref_type__ &operator=(A__ a__) throw() { \
            static_assert(A__::IS_RCU_WRITE_REFERENCE, \
                "Argument to operator= must be an RCU write reference type."); \
            this->internal_pointer__ = a__.get_reference(secret_type__()); \
            return *this; \
        } \
        \
        EACH(WRITE_, NOTHING, NOTHING, __VA_ARGS__) \
        \
        inline operator bool (void) const throw() { \
            return nullptr != (this->internal_pointer__); \
        } \
    };


/// Make a way of accessing the "skeleton" fields of an RCU-protected
/// structure. We define the skeleton fields to be those that are used for
/// traversing and making the structure of the RCU-protected data structure.
///
/// Importantly, this automatically handles rcu_dereference.
#define READ_RCU_REFERENCE(field_name) \
    struct field_name ## _field__ { \
    private: \
        friend union rcu_read_reference<base_type__>; \
        \
        base_type__ *internal_pointer__; \
        \
        typedef field_name ## _field__  self_type__; \
        typedef decltype((new base_type__)->field_name) field_type__; \
        \
        static_assert(std::is_pointer<field_type__>::value, \
            "The RCU_REFERENCE-defined field (" \
            #field_name \
            ") must have a pointer type."); \
        \
        typedef decltype(**(new field_type__)) value_type__; \
        typedef rcu_read_reference<value_type__> ref_type__; \
        \
    public: \
        enum { \
            IS_RCU_READ_REFERENCE = 1 \
        }; \
        \
        inline field_type__ get_reference(secret_type__) throw() { \
            return rcu::dereference(&(internal_pointer__->field_name)); \
        } \
        \
        inline operator ref_type__ (void) throw() { \
            return ref_type__( \
                rcu::dereference(&(internal_pointer__->field_name))); \
        }; \
    } field_name;


/// Make a way of accessing a value field of an RCU protected structure. This
/// can implicitly convert to an r-value of the field's type.
#define READ_RCU_VALUE(field_name) \
    struct field_name ## _field__  { \
    private: \
        base_type__ *internal_pointer__; \
        \
        typedef field_name ## _field__  self_type__; \
        typedef decltype((new base_type__)->field_name) field_type__; \
        typedef typename std::decay<field_type__>::type decayed_type__; \
    public: \
        inline operator decayed_type__ (void) const throw() { \
            return internal_pointer__->field_name; \
        }; \
    } field_name;


/// Defines the policy for accessing a skeleton field of a write reference.
///
/// Importantly, this automatically handles rcu_assign_pointer.
#define WRITE_RCU_REFERENCE(field_name) \
    struct field_name ## _field__; \
    friend struct field_name ## _field__; \
    struct field_name ## _field__ { \
    private: \
        friend union rcu_write_reference<base_type__>; \
        \
        base_type__ *internal_pointer__; \
        \
        typedef field_name ## _field__  self_type__; \
        typedef decltype((new base_type__)->field_name) field_type__; \
        \
        static_assert(std::is_pointer<field_type__>::value, \
            "The RCU_REFERENCE-defined field (" \
            #field_name \
            ") must have a pointer type."); \
        \
        typedef decltype(**(new field_type__)) value_type__; \
        typedef rcu_write_reference<value_type__> ref_type__; \
        \
    public: \
        enum { \
            IS_RCU_WRITE_REFERENCE = 1 \
        }; \
        \
        inline field_type__ get_reference(secret_type__) throw() { \
            return internal_pointer__->field_name; \
        } \
        \
        inline operator ref_type__ (void) throw() { \
            return ref_type__(internal_pointer__->field_name); \
        }; \
        \
        inline operator bool (void) const throw() { \
            return nullptr != (internal_pointer__->field_name); \
        } \
        \
        template <typename A__> \
        inline self_type__ &operator=(A__ val__) throw() { \
            static_assert(A__::IS_RCU_WRITE_REFERENCE, \
                "Argument to operator= must be an RCU write reference type."); \
            rcu::assign_pointer( \
                &(internal_pointer__->field_name), \
                val__.get_reference(secret_type__())); \
            return *this; \
        } \
        \
    } field_name;


/// Returns an lvalue for a value field within a RCU-protected field for a
/// write reference.
#define WRITE_RCU_VALUE(field_name) \
    struct field_name ## _field__ { \
    private: \
        base_type__ *internal_pointer__; \
        typedef field_name ## _field__  self_type__; \
        typedef decltype((new base_type__)->field_name) field_type__; \
        \
    public: \
        inline operator field_type__ &(void) throw() { \
            return internal_pointer__->field_name; \
        }; \
        \
        template <typename A__> \
        inline self_type__ &operator=(A__ val__) throw() { \
            internal_pointer__->field_name = val__; \
            return *this; \
        } \
        \
    } field_name;


    namespace rcu {

        /// De-reference a pointer from within a read reference.
        template <typename T>
        inline T *dereference(register T **ptr) throw() {
            std::atomic_thread_fence(std::memory_order_acquire);
            return *ptr;
        }


        /// Assign to a pointer from within a write reference.
        template <typename T>
        inline void assign_pointer(register T **ptr, register T *new_val) throw() {
            *ptr = new_val;
            std::atomic_thread_fence(std::memory_order_release);
        }


        /// Represents a reference counter that is specific to RCU. RCU
        /// reference counters are organised into a data-structure-specific
        /// hazard pointer list after they are swapped out. The writer swaps
        /// reference counters, then queues up old reference counters into the
        /// list. This list is used by future writers to look for hazards
        /// (reader threads that have a reference to the counter) so that if
        /// the writer finds no hazards then the writer can re-use the counter
        /// for a later read generation.
        struct reference_counter {
        public:

            /// The count of this reference counter.
            std::atomic<unsigned> counter;

            /// Next reference counter in the hazard list / free list.
            reference_counter *next;

            reference_counter(void) throw()
                : counter(ATOMIC_VAR_INIT(0U))
                , next(nullptr)
            { }

            /// Check if the counter is valid.
            inline bool is_valid(void) throw() {
                return 0U == (counter.load() & 1U);
            }

            /// Increment the reference counter; this increments by two so that
            /// the reference counter is always even (when valid) and always
            /// odd (when stale). Returns true iff this reference counter is
            /// valid.
            inline bool increment(void) throw() {
                return 0U == (counter.fetch_add(2) & 1U);
            }

            /// Decrement the reference counter.
            inline void decrement(void) throw() {
                counter.fetch_sub(2);
            }

            /// Wait for the reference counter to hit zero, then swap to one,
            /// thus converting it from valid to stale.
            void wait(void) throw() {
                unsigned expected(0U);

                for(;;) {
                    if(expected != counter.load()) {
                        // TODO: yielding the thread might be good instead of
                        //       spinning needlessly
                        continue;
                    }

                    if(counter.compare_exchange_weak(expected, 1U)) {
                        break;
                    }
                }
            }

            /// Reset the reference counter. Note: this sets the next pointer
            /// to null.
            inline void reset(void) throw() {
                counter.store(0U);
                next = nullptr;
            }
        };
    }


    /// Used to signal that the internal data structure of an RCU-protected
    /// structure should not be initialised.
    static struct { } RCU_INIT_NULL;


    /// Represents a function that can only be used during the `while_readers_
    /// exist` method of an `rcu_writer` for publishing a new version of the
    /// data structure.
    template <typename T>
    struct rcu_publisher {
    private:

        friend struct rcu_protected<T>;

        T **data;

        rcu_publisher(T **data_) throw()
            : data(data_)
        { }

        rcu_publisher(const rcu_publisher<T> &) throw() = delete;
        rcu_publisher<T> &operator=(const rcu_publisher<T> &) throw() = delete;

    public:

        typedef rcu_write_reference<T> write_ref_type;

        ~rcu_publisher(void) throw() {
            data = nullptr;
        }

        /// Publish a new version of the data structure and return the old
        /// version.
        inline T *publish(write_ref_type new_version) throw() {
            std::atomic_thread_fence(std::memory_order_acquire);
            T *old_data(*data);
            *data = new_version.internal_pointer__;
            std::atomic_thread_fence(std::memory_order_release);
            return old_data;
        }

        /// Promote an untracked pointer into a write reference.
        template <typename R>
        inline rcu_write_reference<R> promote(R *ptr) throw() {
            return rcu_write_reference<R>(ptr);
        }

        /// Promote a null pointer into a write reference.
        inline write_ref_type promote(std::nullptr_t) throw() {
            return write_ref_type(nullptr);
        }
    };


    /// Allows demoting of a write reference into a bare pointer. Thus, this
    /// allows us to garbage collect no longer visible write references.
    template <typename T>
    struct rcu_collector {
    private:

        friend struct rcu_protected<T>;

        rcu_collector(void) throw() { }
        rcu_collector(const rcu_collector<T> &) throw() = delete;
        rcu_collector<T> &operator=(const rcu_collector<T> &) throw() = delete;

    public:

        typedef rcu_write_reference<T> write_ref_type;

        /// Convert a write reference into a bare pointer for use in garbage
        /// collecting.
        ///
        /// Note: this modifies the reference in place, making it unusable after
        ///       being demoted.
        template <typename R>
        R *demote(rcu_write_reference<R> &ref) throw() {
            R *ptr(ref.internal_pointer__);
            ref.internal_pointer__ = nullptr;
            return ptr;
        }
    };


    /// Represents the protocol that an RCU writer must follow.
    template <typename T>
    struct rcu_writer {
    public:

        typedef rcu_write_reference<T> write_ref_type;
        typedef rcu_publisher<T> publisher_type;
        typedef rcu_collector<T> collector_type;

        virtual ~rcu_writer(void) throw() { }

        /// This function is executed before mutual exclusion is acquired over
        /// the data structure.
        virtual void setup(void) throw() { }

        /// This function is called after mutual exclusion has been acquired by
        /// the writer (and so this the only writer writing to the data
        /// structure), but before we have waited for all readers to complete
        /// their read-critical sections. This is the only function in which
        /// a user of RCU can publish a new version of the entire data
        /// structure.
        virtual void while_readers_exist(
            write_ref_type, publisher_type &) throw() { }

        /// This function is called after all readers are done completing their
        /// read-critical sections, but before the writer releases its lock on
        /// the data structure.
        virtual void after_readers_done(write_ref_type) throw() { }

        /// This function is called after the writer releases its lock on the
        /// data structure.
        virtual void teardown(collector_type &) throw() { }
    };


    /// Represents a simple spinlock for an RCU-protected data structure.
    /// This can be partially specialised to allow for different data
    /// structures to use different mutex types.
    template <typename T>
    struct rcu_writer_lock : public spin_lock { };


    /// Represents "secret" information used to ease passing pointers without
    /// leaking pointers to users of the RCU API.
    struct rcu_secret {
    private:

        /// Secret tag type used for extracting and communicating pointers.
        struct write_secret_type { };
        struct read_secret_type { };

        /// Make the secret type available to read and write references.
        template <typename> friend union rcu_read_reference;
        template <typename> friend union rcu_write_reference;
    };


    /// Represents an RCU-protected data structure.
    template <typename T>
    struct rcu_protected {
    private:

        /// The protected data.
        mutable T *data;

        /// Lock used by writer threads to
        rcu_writer_lock<T> writer_lock;

        /// Active reference counter.
        std::atomic<rcu::reference_counter *> reader_counter;

        /// List of free and hazard reference counters. Both of these are
        /// implicitly protected by the writer lock, so that we really have
        /// data-structure-specific hazard pointers, rather than thread-
        /// specific hazard pointers.
        rcu::reference_counter *free_counters;
        rcu::reference_counter *hazard_counters;

        /// List of hazardous read counters
        mutable hazard_pointer_list<rcu::reference_counter> active_counters;

        /// Allocate a reference counter.
        rcu::reference_counter *allocate_counter(void) throw() {
            rcu::reference_counter *next_counter(free_counters);
            if(next_counter) {
                free_counters = next_counter->next;
                next_counter->reset();
            } else {
                next_counter = new rcu::reference_counter;
            }
            return next_counter;
        }

        /// Try to free a reference counter.
        void try_retire_reference_counter(void) throw() {
            rcu::reference_counter **prev_ptr(&hazard_counters);
            rcu::reference_counter *hazard(hazard_counters);

            for(;;) {
                for(; hazard; ) {

                    // might not be any readers on this counter because it's at
                    // one; we'll commit to trying this one.
                    if(1 == hazard->counter.load()) {
                        break;
                    }

                    prev_ptr = &(hazard->next);
                    hazard = hazard->next;
                }

                if(!hazard) {
                    return;

                // quick double check ;-)
                } else if(1 == hazard->counter.load()) {
                    break;
                }
            }

            // it is dead because no reader threads have it as a hazardous
            // pointer; add it to the free list.
            if(!active_counters.contains(hazard)) {
                *prev_ptr = hazard->next;
                hazard->next = free_counters;
                free_counters = hazard;
            }
        }

    public:

        typedef rcu_read_reference<T> read_ref_type;
        typedef rcu_write_reference<T> write_ref_type;
        typedef rcu_publisher<T> publisher_type;
        typedef rcu_collector<T> collector_type;

        /// Constructors

        rcu_protected(void) throw()
            : data(new T)
            , reader_counter(ATOMIC_VAR_INIT(new rcu::reference_counter))
            , free_counters(nullptr)
            , hazard_counters(nullptr)
            , active_counters()
        {
            if(std::is_trivial<T>::value) {
                memset(data, 0, sizeof *data);
            }
        }

        template <typename... Args>
        rcu_protected(Args... args) throw()
            : data(new T(args...))
            , reader_counter(ATOMIC_VAR_INIT(new rcu::reference_counter))
            , free_counters(nullptr)
            , hazard_counters(nullptr)
            , active_counters()
        { }

        rcu_protected(decltype(RCU_INIT_NULL)) throw()
            : data(nullptr)
            , reader_counter(ATOMIC_VAR_INIT(new rcu::reference_counter))
            , free_counters(nullptr)
            , hazard_counters(nullptr)
            , active_counters()
        { }

        /// Destructor.
        ~rcu_protected(void) throw() {
            if(data) {
                delete data;
                data = nullptr;
            }

            // delete active counter
            rcu::reference_counter *c(reader_counter.exchange(nullptr));
            if(c) {
                delete c;
            }

            // delete free list
            c = free_counters;
            for(; free_counters; free_counters = c) {
                c = free_counters->next;
                delete free_counters;
            }

            // delete hazard list
            c = hazard_counters;
            for(; hazard_counters; hazard_counters = c) {
                c = hazard_counters->next;
                delete hazard_counters;
            }
        }

        /// Write to the RCU-protected data structure.
        void write(rcu_writer<T> &writer) throw() {
            writer.setup();
            writer_lock.acquire();

            // get the next reference counter and swap it in to be the active
            // reference counters for new readers
            rcu::reference_counter *next_counter(allocate_counter());

            publisher_type publisher(&data);
            writer.while_readers_exist(write_ref_type(data), publisher);

            // exchange the reference counter; if the writer publishes something
            // then some readers will see it and others won't; just to be sure,
            // we conservatively wait for all that might.
            rcu::reference_counter *current_counter(
                reader_counter.exchange(next_counter));

            // before waiting on the current counter, try to release some old
            // hazardous counters to the free list.
            try_retire_reference_counter();

            // wait on the reference counter; hopefully by the time we're done
            // looking for potentially dead hazard pointers, we have "waited"
            // long enough.
            current_counter->wait();

            // pass in a new write ref because a new version of data might have
            // been published in `while_readers_exist`.
            writer.after_readers_done(write_ref_type(data));

            // add the old counter to the hazard list
            current_counter->next = hazard_counters;
            hazard_counters = current_counter;

            writer_lock.release();

            collector_type collector;
            writer.teardown(collector);
        }

        /// Read from the RCU-protected data structure. Invokes a function as
        /// the read-critical section.
        template <typename R, typename... Args>
        inline R read(
            R (*reader_func)(read_ref_type, Args&...),
            Args&... args
        ) const throw() {
            function_call<R, read_ref_type, Args&...> reader(reader_func);
            return read(reader, args...);
        }

    private:

        /// Read from the RCU-protected data structure.
        template <typename R, typename... Args>
        R read(
            function_call<R, read_ref_type, Args&...> reader,
            Args&... args
        ) const throw() {
            rcu::reference_counter *read_counter(nullptr);
            hazard_pointer<rcu::reference_counter> &hp(
                active_counters.acquire());

            for(;;) {
                do {
                    read_counter = reader_counter.load();
                    hp.remember(read_counter);
                } while(read_counter != reader_counter.load());

                // do a read of the counter before we commit to a write of it,
                // because the counter might already be dead.
                if(!read_counter->is_valid()) {
                    continue;
                }

                // it's now stored as a hazard pointer, so it's safe to use.
                if(read_counter->increment()) {
                    break;
                }

                // if the counter is invalid, then decrement it (signaling to
                // writers that some counter is potentially okay to retire)
                // and try to get the counter again.
                read_counter->decrement();
            }

            // invoke the read-critical section
            std::atomic_thread_fence(std::memory_order_seq_cst);
            read_ref_type read_ref(data);
            reader(read_ref, args...);

            read_counter->decrement();
            hp.release();

            return reader.yield();
        }
    };
}}


#endif /* GRANARY_RCU_H_ */
