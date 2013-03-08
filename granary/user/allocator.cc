/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/detach.h"

#include <cstdlib>
#include <stdint.h>

#define PROT_ALL (~0)

extern "C" {

#   include <sys/mman.h>
#   include <unistd.h>

    /// DynamoRIO-compatible heap allocation

    void *heap_alloc(void *, unsigned long long size) {
        granary::cpu_state_handle cpu;
        return cpu->transient_allocator.allocate_untyped(16, size);
    }

    void heap_free(void *, void *addr, unsigned long long) {
#if CONFIG_PRECISE_ALLOCATE
        granary::detail::global_free(addr);
#endif
        (void) addr;
    }


    /// Return temporarily allocated space for an instruction.
    void *heap_alloc_temp_instr(void) {
        granary::cpu_state_handle cpu;
        return cpu->instruction_allocator.allocate_array<uint8_t>(32);
    }


    /// (un)protect up to two pages
    static int make_page_executable(void *page_start) {

        return -1 != mprotect(
            page_start,
            granary::PAGE_SIZE,
            PROT_EXEC | PROT_READ | PROT_WRITE
        );
    }
}

#ifndef MAP_ANONYMOUS
#   ifdef MAP_ANON
#       define MAP_ANONYMOUS MAP_ANON
#   else
#       define MAP_ANONYMOUS 0
#   endif
#endif

#ifndef MAP_SHARED
#   define MAP_SHARED 0
#endif

namespace granary { namespace detail {

    void *global_allocate_executable(unsigned long size) throw() {

        // shared map just in case we manage to get user space instrumentation
        // working in a more general case.
        void *allocated(mmap(
            nullptr,
            size + ALIGN_TO(size, PAGE_SIZE),
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_ANONYMOUS | MAP_PRIVATE,
            -1,
            0));

        if(MAP_FAILED == allocated) {
            granary_break_on_fault();
            granary_fault();
        }

        return allocated;
    }


    void global_free_executable(void *addr, unsigned long size) throw() {
        munmap(addr, size);
    }


    void *global_allocate(unsigned long size) throw() {
        return calloc(1, size);
    }

    void global_free(void *addr) throw() {
        free(addr);
    }
}}

/// Make sure that we always detach on mallocs.
#if defined(GRANARY_USE_PIC) && defined(__linux)
#   ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#   endif
#   include <dlfcn.h>

    // malloc is defined in linux.ld.so and libc.so.
    GRANARY_DYNAMIC_DETACH_POINT(__GI___libc_malloc)
    GRANARY_DYNAMIC_DETACH_POINT(__GI___libc_free)
    GRANARY_DYNAMIC_DETACH_POINT(__libc_malloc)
    GRANARY_DYNAMIC_DETACH_POINT(__libc_calloc)
    GRANARY_DYNAMIC_DETACH_POINT(__libc_free)
#endif

#ifdef GRANARY_USE_PIC
    extern "C" {
        extern void _Znwm(void);
        extern void _Znam(void);
        extern void _ZdlPv(void);
        //extern void _ZdaPv(void);
    }

    /// Make sure that the global operator new/delete are detach points.
    GRANARY_DETACH_POINT(_Znwm) // operator new
    GRANARY_DETACH_POINT(_Znam) // operator new[]
    GRANARY_DETACH_POINT(_ZdlPv) // operator delete
    //GRANARY_DETACH_POINT(_ZdaPv) // operator delete[]

#   if defined(__linux) && 0
#       ifndef _GNU_SOURCE
#           define _GNU_SOURCE
#       endif
#       include <dlfcn.h>

        STATIC_INITIALISE({
            static void *libc_mem_align(dlsym(RTLD_NEXT, "__libc_memalign"));
            if(libc_mem_align) {
                static granary::function_wrapper wrapper = { \
                    reinterpret_cast<granary::app_pc>(libc_mem_align), \
                    reinterpret_cast<granary::app_pc>(libc_mem_align), \
                    "__libc_memalign" \
                }; \
                granary::add_detach_target(wrapper); \
            }
        })

#   endif
#endif


/// Add some illegal detach points.
GRANARY_DETACH_POINT_ERROR(granary::detail::global_allocate)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_allocate_executable)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_free)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_free_executable)
