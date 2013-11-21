/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * detach.cc
 *
 *  Created on: 2013-11-19
 *      Author: Peter Goodman
 */

#include "granary/detach.h"
#include "granary/utils.h"
#include "granary/state.h"
#include "granary/cpu_code_cache.h"

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace granary {


    static void add_detach_alias(
        void *handle,
        const char *alias,
        function_wrapper &wrapper
    ) throw() {
        void *detach_addr(dlsym(handle, alias));
        if(!detach_addr || wrapper.original_address == detach_addr) {
            return;
        }

        app_pc detach_app_pc(unsafe_cast<app_pc>(detach_addr));

        // Note: We don't check if we've already got the entry, because if
        //       added a detach version in one place, then we want to overwrite
        //       it with

        if(wrapper.app_wrapper_address) {
            add_detach_target(
                detach_app_pc,
                wrapper.app_wrapper_address,
                RUNNING_AS_APP);
        }

        if(wrapper.host_wrapper_address) {
            add_detach_target(
                detach_app_pc,
                wrapper.host_wrapper_address,
                RUNNING_AS_HOST);
        }
    }


    static void add_detach_entry(void *handle, const char *name) throw() {
        void *detach_addr(dlsym(handle, name));
        app_pc detach_app_pc(unsafe_cast<app_pc>(detach_addr));

        if(!find_detach_target(detach_app_pc, RUNNING_AS_APP)) {
            add_detach_target(
                detach_app_pc,
                detach_app_pc,
                RUNNING_AS_APP);
        }

        if(!find_detach_target(detach_app_pc, RUNNING_AS_HOST)) {
            add_detach_target(
                detach_app_pc,
                detach_app_pc,
                RUNNING_AS_HOST);
        }
    }


#if CONFIG_FEATURE_WRAPPERS
#   define WRAP_FOR_DETACH(func)
#   define WRAP_ALIAS(func, alias) do { \
        add_detach_alias( \
            handle, \
            TO_STRING(alias), \
            FUNCTION_WRAPPERS[CAT(DETACH_ID_, func)] \
        ); \
    } while(0);
#   define DETACH(func) add_detach_entry(handle, TO_STRING(func));
#   define TYPED_DETACH(func)
#endif


    /// Build up the map file name without depending on any of the cstring
    /// functions.
    static void make_map_file_name(uint64_t pid, char *buff) throw() {
        memcpy(buff, "/proc/", 6);
        buff += 6;
        if(!pid) {
            *buff++ = '0';
        } else {
            uint64_t max_base(10);
            for(; pid / max_base; max_base *= 10) {
                // loop
            }
            for(max_base /= 10; max_base; max_base /= 10) {
                const uint64_t digit(pid / max_base);
                *buff++ = digit + '0';
                pid -= digit * max_base;
            }
        }

        memcpy(buff, "/maps", 6); // including null-terminator
    }


    /// Parse the /proc/<pid>/maps file and apply a function to each path in
    /// the mapping.
    static void parse_proc_maps(
        void (*for_each_path)(const char *, unsigned, char *),
        char *last_path
    ) throw() {
        char map_file[30];
        make_map_file_name(getpid(), &(map_file[0]));

        const int fd(open(&(map_file[0]), O_RDONLY));
        if(-1 == fd) {
            return;
        }

        enum {
            LINE_SIZE = 255
        };

        char path_buff[LINE_SIZE + 1] = {'\0'};
        char read_buff[LINE_SIZE + 1] = {'\0'};

        bool can_read(true);
        bool read_buff_empty(true);
        bool find_path(true);

        char *read_ch(nullptr);
        char *path_ch(nullptr);

        for(unsigned num_bytes(0), path_len(0); ;) {

            // Fill up the read buffer if necessary.
            if(can_read && read_buff_empty) {
                const unsigned read_num_bytes(
                    read(fd, &(read_buff[0]), LINE_SIZE));
                num_bytes += read_num_bytes;
                can_read = read_num_bytes > 0;
                read_buff_empty = false;

                read_buff[read_num_bytes] = '\0';
                read_ch = &(read_buff[0]);
            }

            // Try to find the beginning of a path.
            if(find_path) {

                // We're looking for a path, but we need to re-fill the read
                // buff.
                if(read_buff_empty) {
                    if(!can_read) {
                        break;
                    }
                    continue;
                }

                for(; *read_ch && '/' != *read_ch; ++read_ch) { }

                // No path in the read buff; refill it.
                if('/' != *read_ch) {
                    read_buff_empty = true;
                    continue;
                }

                // We've found the beginning of a path; try to complete it.
                path_ch = &(path_buff[0]);
                for(path_len = 0; *read_ch && '\n' != *read_ch; ++path_len) {
                    *path_ch++ = *read_ch++;
                    *path_ch = '\0';
                }

                // We completed a path.
                if('\n' == *read_ch) {
                    for_each_path(path_buff, path_len, last_path);
                    path_len = 0;
                } else {
                    find_path = false;
                }

            // Try to complete a path.
            } else {
                for(; *read_ch && '\n' != *read_ch; ++path_len) {
                    *path_ch++ = *read_ch++;
                    *path_ch = '\0';
                }

                // We completed a path.
                if('\n' == *read_ch) {
                    find_path = true;

                    for_each_path(path_buff, path_len, last_path);
                    path_len = 0;

                // Need more characters for the path.
                } else {
                    read_buff_empty = true;
                }
            }
        }

        close(fd);
    }


    /// Visit a potential dynamic (or static) library, and try to find detach
    /// addresses.
    static void visit_elf_path(
        const char *path,
        unsigned path_len,
        char *last_path
    ) throw() {

        // Don't re-visit the same ELF path name twice in a row.
        if(0 == memcmp(path, last_path, path_len)) {
            return;
        } else {
            memcpy(last_path, path, path_len);
        }

        void *handle(dlopen(path, RTLD_NOW));
        if(nullptr == handle) {
            return;
        }

#if CONFIG_FEATURE_WRAPPERS
#   include "granary/gen/user_detach.inc"
#endif

        dlclose(handle);
    }


    STATIC_INITIALISE_ID(init_user_detach, {
        char last_path[256] = {'\0'};
        parse_proc_maps(visit_elf_path, &(last_path[0]));
    })
}
