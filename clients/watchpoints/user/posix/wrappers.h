/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-05-22
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_WRAPPERS_H_
#define WATCHPOINT_WRAPPERS_H_

#include "clients/watchpoints/instrument.h"

#ifdef CLIENT_WATCHPOINT_WATCHED
#   include "clients/watchpoints/clients/everything_watched/user/posix/wrappers.h"
#endif


#ifdef CLIENT_WATCHPOINT_STATS
#   include "clients/watchpoints/clients/everything_watched/user/posix/wrappers.h"
#endif


#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/clients/bounds_checker/user/posix/wrappers.h"
#endif


using namespace client::wp;


#if defined(CAN_WRAP_stat) && CAN_WRAP_stat
#   define APP_WRAPPER_FOR_stat
    FUNCTION_WRAPPER(APP, stat, (int), (const char *path, struct stat *buf), {
        return stat(unwatched_address_check(path), unwatched_address_check(buf));
    })
#endif


/// To wrap stat you actually have to wrap __xstat{,64} as that is the symbol
/// that will actually get linked in.
///
/// TODO: This is likely platform specific and super fragile because the
///       function prototypes don't match:
///         __xstat{,64}(int vers, const char * file, struct stat{,64} *buf)
///         stat(const char *path, struct stat *buf)
///
/// Note: We assume that if code is using __xstat64, that __xstat is an alias
///       for that symbol.
#if defined(CAN_WRAP___xstat) && CAN_WRAP___xstat
#   define APP_WRAPPER_FOR___xstat
    FUNCTION_WRAPPER(APP, __xstat, (int), (int vers, const char *path, struct stat *buf), {
        return __xstat(vers, unwatched_address_check(path), unwatched_address_check(buf));
    })
#endif


#if defined(CAN_WRAP_lstat) && CAN_WRAP_lstat
#   define APP_WRAPPER_FOR_lstat
    FUNCTION_WRAPPER(APP, lstat, (int), (const char *path, struct stat *buf), {
        return lstat(unwatched_address_check(path), unwatched_address_check(buf));
    })
#endif


#if defined(CAN_WRAP___lxstat) && CAN_WRAP___lxstat
#   define APP_WRAPPER_FOR___lxstat
    FUNCTION_WRAPPER(APP, __lxstat, (int), (int vers, const char *path, struct stat *buf), {
        return __lxstat(vers, unwatched_address_check(path), unwatched_address_check(buf));
    })
#endif


/// Technically this function is overloaded in the source spec (i.e. it can
/// take an optional permissions argument for if something is created). I'm not
/// actually sure how this is supported in the source interface (c99 doesn't
/// allow overloaded functions, are they using weak refs?) so I think the name
/// that gets linked would be the same =/
///
/// Note: testfs doesn't use the permissions flag so I am implementing that
///       version of the function.
#if defined(CAN_WRAP_open) && CAN_WRAP_open
#   define APP_WRAPPER_FOR_open
    FUNCTION_WRAPPER(APP, open, (int), (const char *file, int flags, mode_t mode), {
        return open(unwatched_address_check(file), flags, mode);
    })
#endif


#if defined(CAN_WRAP_openat) && CAN_WRAP_openat
#   define APP_WRAPPER_FOR_openat
    FUNCTION_WRAPPER(APP, openat, (int), (int dirfd, const char *file, int flags, mode_t mode), {
        return openat(dirfd, unwatched_address_check(file), flags, mode);
    })
#endif


#if defined(CAN_WRAP_readlink) && CAN_WRAP_readlink
#   define APP_WRAPPER_FOR_readlink
    FUNCTION_WRAPPER(APP, readlink, (ssize_t), (const char *path , char *buf , size_t len), {
        return readlink(unwatched_address_check(path), unwatched_address_check(buf), len);
    })
#endif


#if defined(CAN_WRAP_dlopen) && CAN_WRAP_dlopen
#   define APP_WRAPPER_FOR_dlopen
    FUNCTION_WRAPPER(APP, dlopen, (void *), (const char *filename, int flag), {
        return dlopen(unwatched_address_check(filename), flag);
    })
#endif


#if defined(CAN_WRAP_dlsym) && CAN_WRAP_dlsym
#   define APP_WRAPPER_FOR_dlsym
    FUNCTION_WRAPPER(APP, dlsym, (void *), (void *handle, const char* symbol), {
        return dlsym(unwatched_address_check(handle), unwatched_address_check(symbol));
    })
#endif


/// The exec* calls pass argv and possibly envp which are lists of pointers.
/// Any pointer in the list could be tainted so we must fix them.

#define FIX_LIST_VAR(list) \
    for(char **ptr(list); *ptr; ++ptr) { \
        *ptr = unwatched_address_check(*ptr); \
    }


#if defined(CAN_WRAP_execve) && CAN_WRAP_execve
#   define APP_WRAPPER_FOR_execve
    FUNCTION_WRAPPER(APP, execve, (int), (const char *filename, char *argv[], char *envp[]), {
        FIX_LIST_VAR(argv)
        FIX_LIST_VAR(envp)
        return execve(
            unwatched_address_check(filename),
            unwatched_address_check(argv),
            unwatched_address_check(envp));
    })
#endif


#if defined(CAN_WRAP_execv) && CAN_WRAP_execv
#   define APP_WRAPPER_FOR_execv
    FUNCTION_WRAPPER(APP, execv, (int), (const char *path, char **argv), {
        FIX_LIST_VAR(argv)
        return execv(unwatched_address_check(path), unwatched_address_check(argv));
    })
#endif


#if defined(CAN_WRAP_execvp) && CAN_WRAP_execvp
#   define APP_WRAPPER_FOR_execvp
    FUNCTION_WRAPPER(APP, execvp, (int), (const char *file, char **argv), {
        FIX_LIST_VAR(argv)
        return execvp(unwatched_address_check(file), unwatched_address_check(argv));
    })
#endif


#if defined(CAN_WRAP_execvpe) && CAN_WRAP_execvpe
#   define APP_WRAPPER_FOR_execvpe
    FUNCTION_WRAPPER(APP, execvpe, (int), (const char *file, char *argv[], char *envp[]), {
        FIX_LIST_VAR(argv)
        FIX_LIST_VAR(envp)
        return execvpe(
            unwatched_address_check(file),
            unwatched_address_check(argv),
            unwatched_address_check(envp));
    })
#endif


#if defined(CAN_WRAP_fopen) && CAN_WRAP_fopen
#   define APP_WRAPPER_FOR_fopen
    FUNCTION_WRAPPER(APP, fopen, (FILE *), (const char *path, const char *mode), {
        return fopen(unwatched_address_check(path), unwatched_address_check(mode));
    })
#endif


#if defined(CAN_WRAP_read) && CAN_WRAP_read
#   define APP_WRAPPER_FOR_read
    FUNCTION_WRAPPER(APP, read, (ssize_t), (int fd, void *buf, size_t count), {
        return read(fd, unwatched_address_check(buf), count);
    })
#endif


#if defined(CAN_WRAP_write) && CAN_WRAP_write
#   define APP_WRAPPER_FOR_write
    FUNCTION_WRAPPER(APP, write, (ssize_t), (int fd, const void *buf, size_t count), {
        return write(fd, unwatched_address_check(buf), count);
    })
#endif


#if defined(CAN_WRAP_getc) && CAN_WRAP_getc
#   define APP_WRAPPER_FOR_getc
    FUNCTION_WRAPPER(APP, getc, (int), (FILE *stream), {
        return getc(unwatched_address_check(stream));
    })
#endif


#ifndef APP_WRAPPER_FOR_pointer
#   define APP_WRAPPER_FOR_pointer
    POINTER_WRAPPER({
        INHERIT_PRE_IN
        PRE_OUT {
            if(is_valid_address(arg)) {
                arg = unwatched_address_check(arg);
                PRE_OUT_WRAP(*arg);
            }
        }
        INHERIT_POST_INOUT
        INHERIT_RETURN_INOUT
    })
#endif


#define WP_BASIC_POINTER_WRAPPER(base_type) \
    TYPE_WRAPPER(base_type *, { \
        NO_PRE_IN \
        PRE_OUT { \
            arg = unwatched_address_check(arg); \
        } \
        NO_POST \
        NO_RETURN \
    })


#ifndef APP_WRAPPER_FOR_void_pointer
#   define APP_WRAPPER_FOR_void_pointer
    WP_BASIC_POINTER_WRAPPER(void)
#endif


#ifndef APP_WRAPPER_FOR_int8_t_pointer
#   define APP_WRAPPER_FOR_int8_t_pointer
    static_assert(sizeof(char) == sizeof(int8_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(char)
#endif


#ifndef APP_WRAPPER_FOR_uint8_t_pointer
#   define APP_WRAPPER_FOR_uint8_t_pointer
    static_assert(sizeof(char) == sizeof(int8_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned char)
#endif


#ifndef APP_WRAPPER_FOR_int16_t_pointer
#   define APP_WRAPPER_FOR_int16_t_pointer
    static_assert(sizeof(short) == sizeof(int16_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(short)
#endif


#ifndef APP_WRAPPER_FOR_uint16_t_pointer
#   define APP_WRAPPER_FOR_uint16_t_pointer
    static_assert(sizeof(short) == sizeof(int16_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned short)
#endif


#ifndef APP_WRAPPER_FOR_int32_t_pointer
#   define APP_WRAPPER_FOR_int32_t_pointer
    static_assert(sizeof(int) == sizeof(int32_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(int)
#endif


#ifndef APP_WRAPPER_FOR_uint32_t_pointer
#   define APP_WRAPPER_FOR_uint32_t_pointer
    static_assert(sizeof(int) == sizeof(int32_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned)
#endif


#ifndef APP_WRAPPER_FOR_int64_t_pointer
#   define APP_WRAPPER_FOR_int64_t_pointer
    static_assert(sizeof(long) == sizeof(int64_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(long)
#endif


#ifndef APP_WRAPPER_FOR_uint64_t_pointer
#   define APP_WRAPPER_FOR_uint64_t_pointer
    static_assert(sizeof(long) == sizeof(int64_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned long)
#endif


#ifndef APP_WRAPPER_FOR_float_pointer
#   define APP_WRAPPER_FOR_float_pointer
    WP_BASIC_POINTER_WRAPPER(float)
#endif


#ifndef APP_WRAPPER_FOR_double_pointer
#   define APP_WRAPPER_FOR_double_pointer
    WP_BASIC_POINTER_WRAPPER(double)
#endif


#endif /* WATCHPOINT_WRAPPERS_H_ */
