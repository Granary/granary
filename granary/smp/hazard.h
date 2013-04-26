/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * hazard.h
 *
 *  Created on: 2013-01-18
 *      Author: pag
 */

#ifndef GRANARY_HAZARD_H_
#define GRANARY_HAZARD_H_

namespace granary { namespace smp {

    /// Forward declarations.
    template <typename> struct hazard_pointer_list;


    /// Represents a shared list of hazard pointers.
    template <typename T>
    struct hazard_pointer {
    private:

        friend struct hazard_pointer_list<T>;

        /// Next hazard pointer in the list
        hazard_pointer<T> *next;

        /// Is this hazard pointer active? I.e. should the pointer
        /// contained be considered as one that is held by another thread.
        std::atomic<bool> is_active;

        /// The pointer held by this hazard pointer structure.
        std::atomic<T *> hazardous_pointer;

    public:

        hazard_pointer(void) throw()
            : next(nullptr)
            , is_active(ATOMIC_VAR_INIT(true))
            , hazardous_pointer(ATOMIC_VAR_INIT(nullptr))
        { }

        /// Update the hazard pointer entry with a pointer value.
        inline void remember(T *ptr) throw() {
            hazardous_pointer.store(ptr);
        }

        /// Release a hazard pointer entry.
        void release(void) throw() {
            this->hazardous_pointer.store(nullptr);
            this->is_active.store(false);
        }
    };


    /// Represents a hazard pointer list.
    template <typename T>
    struct hazard_pointer_list {
    private:

        std::atomic<hazard_pointer<T> *> head;

    public:

        hazard_pointer_list(void) throw()
            : head(ATOMIC_VAR_INIT(nullptr))
        { }

        /// Destructor; delete the hazard pointer list, but NOT the hazard
        /// pointers themselves.
        ~hazard_pointer_list(void) throw() {
            hazard_pointer<T> *ptr(head.exchange(nullptr));
            hazard_pointer<T> *next_ptr(nullptr);

            for(; ptr; ptr = next_ptr) {
                next_ptr = ptr->next;
                delete ptr;
            }
        }

        /// Acquire a new hazard pointer entry.
        hazard_pointer<T> &acquire(void) throw() {

            hazard_pointer<T> *p(head.load());
            bool inactive(false);

            for(; p; p = p->next) {
                if(p->is_active.load()) {
                    continue;
                }

                // only try once for each pointer
                if(!p->is_active.compare_exchange_weak(inactive, true)) {
                    continue;
                }

                return *p;
            }

            // need to allocate a new hazard pointer
            p = new hazard_pointer<T>;
            hazard_pointer<T> *head_hp(nullptr);

            // link this hazard pointer in as the head of the list.
            do {
                head_hp = head.load();
                p->next = head_hp;
            } while(!head.compare_exchange_weak(head_hp, p));

            return *p;
        }

        /// Returns true if a particular pointer is contained in the hazard
        /// pointer list.
        bool contains(const T * const ptr) throw() {
            hazard_pointer<T> *p(head.load());
            for(; p; p = p->next) {
                if(!p->is_active.load()) {
                    continue;
                }

                if(p->hazardous_pointer.load() == ptr) {
                    return true;
                }
            }
            return false;
        }
    };

}}

#endif /* GRANARY_HAZARD_H_ */
