/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_USER_POSIX_OVERRIDE_WRAPPERS_H_
#define granary_USER_POSIX_OVERRIDE_WRAPPERS_H_


/// Disable wrapping of some Mac OS X types.
#define APP_WRAPPER_FOR_struct___sFILE
#define APP_WRAPPER_FOR_struct_malloc_introspection_t
#define APP_WRAPPER_FOR_struct__malloc_zone_t
#define APP_WRAPPER_FOR_glob_t


/// Disable wrapping of some Linux types.
#define APP_WRAPPER_FOR_glob64_t
#define APP_WRAPPER_FOR__IO_cookie_io_functions_t


#if defined(CAN_WRAP_stat) && CAN_WRAP_stat
#   define APP_WRAPPER_FOR_stat
    FUNCTION_WRAPPER(APP, stat, (int), (const char *path, struct stat *buf), {
        return stat(VALID_ADDRESS(path), VALID_ADDRESS(buf));
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
        return __xstat(vers, VALID_ADDRESS(path), VALID_ADDRESS(buf));
    })
#endif


#if defined(CAN_WRAP_lstat) && CAN_WRAP_lstat
#   define APP_WRAPPER_FOR_lstat
    FUNCTION_WRAPPER(APP, lstat, (int), (const char *path, struct stat *buf), {
        return lstat(VALID_ADDRESS(path), VALID_ADDRESS(buf));
    })
#endif


#if defined(CAN_WRAP___lxstat) && CAN_WRAP___lxstat
#   define APP_WRAPPER_FOR___lxstat
    FUNCTION_WRAPPER(APP, __lxstat, (int), (int vers, const char *path, struct stat *buf), {
        return __lxstat(vers, VALID_ADDRESS(path), VALID_ADDRESS(buf));
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
    FUNCTION_WRAPPER(APP, open, (int), (const char *file, int flags, ...), {
        va_list vargs;
        va_start(vargs, flags);
        mode_t mode = va_arg(vargs, int);
        va_end(vargs);

        return open(VALID_ADDRESS(file), flags, mode);
    })
#endif


#if defined(CAN_WRAP_openat) && CAN_WRAP_openat
#   define APP_WRAPPER_FOR_openat
    FUNCTION_WRAPPER(APP, openat, (int), (int dirfd, const char *file, int flags, ...), {
        va_list vargs;
        va_start(vargs, flags);
        mode_t mode = va_arg(vargs, int);
        va_end(vargs);

        return openat(dirfd, VALID_ADDRESS(file), flags, mode);
    })
#endif


#if defined(CAN_WRAP_readlink) && CAN_WRAP_readlink
#   define APP_WRAPPER_FOR_readlink
    FUNCTION_WRAPPER(APP, readlink, (ssize_t), (const char *path , char *buf , size_t len), {
        return readlink(VALID_ADDRESS(path), VALID_ADDRESS(buf), len);
    })
#endif


#if defined(CAN_WRAP_dlopen) && CAN_WRAP_dlopen
#   define APP_WRAPPER_FOR_dlopen
    FUNCTION_WRAPPER(APP, dlopen, (void *), (const char *filename, int flag), {
        return dlopen(VALID_ADDRESS(filename), flag);
    })
#endif


#if defined(CAN_WRAP_dlsym) && CAN_WRAP_dlsym
#   define APP_WRAPPER_FOR_dlsym
    FUNCTION_WRAPPER(APP, dlsym, (void *), (void *handle, const char* symbol), {
        return dlsym(VALID_ADDRESS(handle), VALID_ADDRESS(symbol));
    })
#endif


/// The exec* calls pass argv and possibly envp which are lists of pointers.
/// Any pointer in the list could be tainted so we must fix them.

#define FIX_LIST_VAR(list) \
    for(char **ptr(list); *ptr; ++ptr) { \
        *ptr = VALID_ADDRESS(*ptr); \
    }


#if defined(CAN_WRAP_execve) && CAN_WRAP_execve
#   define APP_WRAPPER_FOR_execve
    FUNCTION_WRAPPER(APP, execve, (int), (const char *filename, char *argv[], char *envp[]), {
        FIX_LIST_VAR(argv)
        FIX_LIST_VAR(envp)
        return execve(
            VALID_ADDRESS(filename),
            VALID_ADDRESS(argv),
            VALID_ADDRESS(envp));
    })
#endif


#if defined(CAN_WRAP_execv) && CAN_WRAP_execv
#   define APP_WRAPPER_FOR_execv
    FUNCTION_WRAPPER(APP, execv, (int), (const char *path, char **argv), {
        FIX_LIST_VAR(argv)
        return execv(VALID_ADDRESS(path), VALID_ADDRESS(argv));
    })
#endif


#if defined(CAN_WRAP_execvp) && CAN_WRAP_execvp
#   define APP_WRAPPER_FOR_execvp
    FUNCTION_WRAPPER(APP, execvp, (int), (const char *file, char **argv), {
        FIX_LIST_VAR(argv)
        return execvp(VALID_ADDRESS(file), VALID_ADDRESS(argv));
    })
#endif


#if defined(CAN_WRAP_execvpe) && CAN_WRAP_execvpe
#   define APP_WRAPPER_FOR_execvpe
    FUNCTION_WRAPPER(APP, execvpe, (int), (const char *file, char *argv[], char *envp[]), {
        FIX_LIST_VAR(argv)
        FIX_LIST_VAR(envp)
        return execvpe(
            VALID_ADDRESS(file),
            VALID_ADDRESS(argv),
            VALID_ADDRESS(envp));
    })
#endif


#if defined(CAN_WRAP_fopen) && CAN_WRAP_fopen
#   define APP_WRAPPER_FOR_fopen
    FUNCTION_WRAPPER(APP, fopen, (FILE *), (const char *path, const char *mode), {
        return fopen(VALID_ADDRESS(path), VALID_ADDRESS(mode));
    })
#endif


#if defined(CAN_WRAP_read) && CAN_WRAP_read
#   define APP_WRAPPER_FOR_read
    FUNCTION_WRAPPER(APP, read, (ssize_t), (int fd, void *buf, size_t count), {
        return read(fd, VALID_ADDRESS(buf), count);
    })
#endif


#if defined(CAN_WRAP_write) && CAN_WRAP_write
#   define APP_WRAPPER_FOR_write
    FUNCTION_WRAPPER(APP, write, (ssize_t), (int fd, const void *buf, size_t count), {
        return write(fd, VALID_ADDRESS(buf), count);
    })
#endif


#if defined(CAN_WRAP_getc) && CAN_WRAP_getc
#   define APP_WRAPPER_FOR_getc
    FUNCTION_WRAPPER(APP, getc, (int), (FILE *stream), {
        return getc(VALID_ADDRESS(stream));
    })
#endif


#if defined(CAN_WRAP_vfork) && CAN_WRAP_vfork && !defined(APP_WRAPPER_FOR_vfork)
#   define APP_WRAPPER_FOR_vfork
    FUNCTION_WRAPPER(APP, vfork, (pid_t), (void), {
        return fork();
    })
#endif


#endif /* granary_USER_POSIX_OVERRIDE_WRAPPERS_H_ */
