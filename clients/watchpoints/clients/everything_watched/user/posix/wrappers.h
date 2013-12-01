/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_wrappers.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_WATCHED_WRAPPERS_H_
#define WATCHPOINT_WATCHED_WRAPPERS_H_


#include "clients/watchpoints/clients/everything_watched/instrument.h"


using namespace client::wp;


#if defined(CAN_WRAP_realloc) && CAN_WRAP_realloc
#   define APP_WRAPPER_FOR_realloc
    FUNCTION_WRAPPER(APP, realloc, (void *), (void *ptr, size_t size), {
        void *new_ptr(realloc(unwatched_address_check(ptr), size));
        if(new_ptr) {
            add_watchpoint(new_ptr);
        }
        return new_ptr;
    })
#endif


#define MALLOCATOR(func, size) \
    { \
        void *ptr(func(size)); \
        if(!ptr) { \
            return ptr; \
        } \
        add_watchpoint(ptr); \
        return ptr; \
    }


#if defined(CAN_WRAP_malloc) && CAN_WRAP_malloc
#   define APP_WRAPPER_FOR_malloc
    FUNCTION_WRAPPER(APP, malloc, (void *), (size_t size), MALLOCATOR(malloc, size))
#endif


#if defined(CAN_WRAP___libc_malloc) && CAN_WRAP___libc_malloc
#   define APP_WRAPPER_FOR___libc_malloc
    FUNCTION_WRAPPER(APP, __libc_malloc, (void *), (size_t size), MALLOCATOR(__libc_malloc, size))
#endif


#if defined(CAN_WRAP_valloc) && CAN_WRAP_valloc
#   define APP_WRAPPER_FOR_valloc
    FUNCTION_WRAPPER(APP, valloc, (void *), (size_t size), MALLOCATOR(valloc, size))
#endif


#if defined(CAN_WRAP___libc_valloc) && CAN_WRAP___libc_valloc
#   define APP_WRAPPER_FOR___libc_valloc
    FUNCTION_WRAPPER(APP, __libc_valloc, (void *), (size_t size), MALLOCATOR(__libc_valloc, size))
#endif


#define CALLOCATOR(func, count, size) \
    { \
        void *ptr(func(count, size)); \
        if(!ptr) { \
            return ptr; \
        } \
        add_watchpoint(ptr); \
        return ptr; \
    }


#if defined(CAN_WRAP_malloc) && CAN_WRAP_calloc
#   define APP_WRAPPER_FOR_calloc
    FUNCTION_WRAPPER(APP, calloc, (void *), (size_t count, size_t size), CALLOCATOR(calloc, count, size))
#endif


#if defined(CAN_WRAP___libc_calloc) && CAN_WRAP___libc_calloc
#   define APP_WRAPPER_FOR___libc_calloc
    FUNCTION_WRAPPER(APP, __libc_calloc, (void *), (size_t count, size_t size), CALLOCATOR(__libc_calloc, count, size))
#endif


#if defined(CAN_WRAP_stat) && CAN_WRAP_stat
#   define APP_WRAPPER_FOR_stat
    FUNCTION_WRAPPER(APP, stat, (int), (const char *path, struct stat *buf), {
		return stat(unwatched_address_check(path), unwatched_address_check(buf));	
    })
#endif


//So to wrap stat you actually have to wrap __xstat{,64} as that is the symbol that will actually get linked in 
//XXX this is likely platform specific and super fragile
//__xstat{,64}  takes (int vers, const char * file, struct stat{,64} *buf)
//stat takes (const char *path, struct stat *buf)
//
//This code is written assuming a 64 bit platform so __xstat == __xstat64. I don't think __xstat could ever
//actually be linked in but the wrapper is here ... just because.
#if defined(CAN_WRAP___xstat) && CAN_WRAP___xstat
#   define APP_WRAPPER_FOR___xstat
    FUNCTION_WRAPPER(APP, __xstat, (int), (int vers, const char *path, struct stat *buf), {
		return stat(unwatched_address_check(path), unwatched_address_check(buf));	
    })
#endif

#if defined(CAN_WRAP___xstat64) && CAN_WRAP___xstat64
#   define APP_WRAPPER_FOR___xstat64
    FUNCTION_WRAPPER(APP, __xstat64, (int), (int vers, const char *path, struct stat *buf), {
		return stat(unwatched_address_check(path), unwatched_address_check(buf));	
    })
#endif

//Technically this function is overloaded in the source spec (i.e. it can take an optional permissions argument for if something is created)
//I'm not actually sure how this is supported in the source interface (c99 doesn't allow overloaded functions, are they using weak refs?) 
//so I think the name that gets linked would be the same =/
//testfs doesn't use the permissions flag so I am implementing that version of the funciton
#if defined(CAN_WRAP_open) && CAN_WRAP_open
#	define APP_WRAPPER_FOR_open
	FUNCTION_WRAPPER(APP, open, (int), (const char *file, int mode), {
		return open(unwatched_address_check(file), mode);
	})
#endif

#if defined(CAN_WRAP_open64) && CAN_WRAP_open64
#	define APP_WRAPPER_FOR_open64
	FUNCTION_WRAPPER(APP, open64, (int), (const char *file, int mode), {
		return open64(unwatched_address_check(file), mode);
	})
#endif

#if defined(CAN_WRAP_dlopen) && CAN_WRAP_dlopen
#	define APP_WRAPPER_FOR_dlopen
	FUNCTION_WRAPPER(APP, dlopen, (void *), (const char *filename, int flag), {
		return dlopen(unwatched_address_check(filename), flag);
	})
#endif

#if defined(CAN_WRAP_dlsym) && CAN_WRAP_dlsym
#	define APP_WRAPPER_FOR_dlsym
	FUNCTION_WRAPPER(APP, dlsym, (void *), (void *handle, const char* symbol), {
		return dlsym(unwatched_address_check(handle), unwatched_address_check(symbol));
	})
#endif


//the exec* calls pass argv and possibly envp which are lists of pointers. 
//Any pointer in the list could be tainted so we must fix them.

#define FIX_LIST_VAR(list) \
{ \
	int i=0; \
	char **ptr(list); \
	char *cur; \
	while((cur = *ptr)) { \
		list[i] = unwatched_address_check(cur); \
		i++; \
		ptr++; \
	} \
}

#if defined(CAN_WRAP_execve) && CAN_WRAP_execve
#	define APP_WRAPPER_FOR_execve
	FUNCTION_WRAPPER(APP, execve, (int), (const char *filename, char *argv[], char *envp[]), {
		FIX_LIST_VAR(argv)
		FIX_LIST_VAR(envp)
		return execve(unwatched_address_check(filename), unwatched_address_check(argv), unwatched_address_check(envp));
	})
#endif

#if defined(CAN_WRAP_execv) && CAN_WRAP_execv
#	define APP_WRAPPER_FOR_execv
	FUNCTION_WRAPPER(APP, execv, (int), (const char *path, char **argv), {
		FIX_LIST_VAR(argv)
		return execv(unwatched_address_check(path), unwatched_address_check(argv));
	})
#endif

#if defined(CAN_WRAP_execvp) && CAN_WRAP_execvp
#	define APP_WRAPPER_FOR_execvp
	FUNCTION_WRAPPER(APP, execvp, (int), (const char *file, char **argv), {
		FIX_LIST_VAR(argv)
		return execvp(unwatched_address_check(file), unwatched_address_check(argv));
	})
#endif

#if defined(CAN_WRAP_execvpe) && CAN_WRAP_execvpe
#	define APP_WRAPPER_FOR_execvpe
	FUNCTION_WRAPPER(APP, execvpe, (int), (const char *file, char *argv[], char *envp[]), {
		FIX_LIST_VAR(argv)
		FIX_LIST_VAR(envp)
		return execvpe(unwatched_address_check(file), unwatched_address_check(argv), unwatched_address_check(envp));
	})
#endif

#if defined(CAN_WRAP_fopen) && CAN_WRAP_fopen
#	define APP_WRAPPER_FOR_fopen
	FUNCTION_WRAPPER(APP, fopen, (FILE *), (const char *path, const char *mode), {
		return fopen(unwatched_address_check(path), unwatched_address_check(mode));
	})
#endif

#if defined(CAN_WRAP_raed) && CAN_WRAP_read
#	define APP_WRAPPER_FOR_read
	FUNCTION_WRAPPER(APP, read, (ssize_t), (int fd, void *buf, size_t count), {
		return read(fd, unwatched_address_check(buf), count);
	})
#endif

#if defined(CAN_WRAP_write) && CAN_WRAP_write
#	define APP_WRAPPER_FOR_write
	FUNCTION_WRAPPER(APP, write, (ssize_t), (int fd, const void *buf, size_t count), {
		return write(fd, unwatched_address_check(buf), count);
	})
#endif

#if defined(CAN_WRAP_fread) && CAN_WRAP_fread
#	define APP_WRAPPER_FOR_fread
	FUNCTION_WRAPPER(APP, fread, (size_t), (void *ptr, size_t size, size_t nmemb, FILE *stream), {
		return fread(unwatched_address_check(ptr), size, nmemb, unwatched_address_check(stream));
	})
#endif

#if defined(CAN_WRAP_getc) && CAN_WRAP_getc
#	define APP_WRAPPER_FOR_getc
	FUNCTION_WRAPPER(APP, getc, (int), (FILE *stream), {
		return getc(unwatched_address_check(stream));
	})
#endif

#endif /* WATCHPOINT_WATCHED_WRAPPERS_H_ */
