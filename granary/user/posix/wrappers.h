/*
 * wrappers.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_USER_POSIX_OVERRIDE_WRAPPERS_H_
#define granary_USER_POSIX_OVERRIDE_WRAPPERS_H_


#define EXECL_ARG(num, last_arg, seen_null, args_arr, args_list) \
    if(!(seen_null)) { \
        (args_arr)[num] = va_arg((args_list), char *); \
        (seen_null) = (nullptr == (args_arr)[num]); \
        if(!(seen_null)) { \
            (last_arg) = (num); \
        } \
    } else { \
        (args_arr)[num] = nullptr; \
    }


#if defined(CAN_WRAP_execl) && defined(CAN_WRAP_execv)
#   define WRAPPER_FOR_execl
    FUNCTION_WRAPPER(execl, (int), (char *__path, char *__arg, ...), {
        granary::printf("execl(%s, %s, ...)\n", __path, __arg);
        va_list args__;
        char *args[12] = {nullptr};
        int last_arg(0);
        args[0] = __arg;
        bool seen_null(false);
        va_start(args__, __arg);
        EXECL_ARG(1, last_arg, seen_null, args, args__)
        EXECL_ARG(2, last_arg, seen_null, args, args__)
        EXECL_ARG(3, last_arg, seen_null, args, args__)
        EXECL_ARG(4, last_arg, seen_null, args, args__)
        EXECL_ARG(5, last_arg, seen_null, args, args__)
        EXECL_ARG(6, last_arg, seen_null, args, args__)
        EXECL_ARG(7, last_arg, seen_null, args, args__)
        EXECL_ARG(8, last_arg, seen_null, args, args__)
        EXECL_ARG(9, last_arg, seen_null, args, args__)
        EXECL_ARG(10, last_arg, seen_null, args, args__)
        va_end(args__);
        (void) last_arg;
        return execv(__path, args);
    })
#endif


#if defined(CAN_WRAP_execlp) && defined(CAN_WRAP_execvp)
#   define WRAPPER_FOR_execlp
    FUNCTION_WRAPPER(execlp, (int), (char *__file, char *__arg, ...), {
        granary::printf("execlp(%s, %s, ...)\n", __file, __arg);
        va_list args__;
        char *args[12] = {nullptr};
        int last_arg(0);
        args[0] = __arg;
        bool seen_null(false);
        va_start(args__, __arg);
        EXECL_ARG(1, last_arg, seen_null, args, args__)
        EXECL_ARG(2, last_arg, seen_null, args, args__)
        EXECL_ARG(3, last_arg, seen_null, args, args__)
        EXECL_ARG(4, last_arg, seen_null, args, args__)
        EXECL_ARG(5, last_arg, seen_null, args, args__)
        EXECL_ARG(6, last_arg, seen_null, args, args__)
        EXECL_ARG(7, last_arg, seen_null, args, args__)
        EXECL_ARG(8, last_arg, seen_null, args, args__)
        EXECL_ARG(9, last_arg, seen_null, args, args__)
        EXECL_ARG(10, last_arg, seen_null, args, args__)
        va_end(args__);
        (void) last_arg;
        return execvp(__file, args);
    })
#endif


#if defined(CAN_WRAP_execle) && defined(CAN_WRAP_execvpe)
#   define WRAPPER_FOR_execle
    FUNCTION_WRAPPER(execle, (int), (char *__path, char *__arg, ...), {
        va_list args__;
        char *args[12] = {nullptr};
        int last_arg(0);
        args[0] = __arg;
        bool seen_null(false);
        va_start(args__, __arg);
        EXECL_ARG(1, last_arg, seen_null, args, args__)
        EXECL_ARG(2, last_arg, seen_null, args, args__)
        EXECL_ARG(3, last_arg, seen_null, args, args__)
        EXECL_ARG(4, last_arg, seen_null, args, args__)
        EXECL_ARG(5, last_arg, seen_null, args, args__)
        EXECL_ARG(6, last_arg, seen_null, args, args__)
        EXECL_ARG(7, last_arg, seen_null, args, args__)
        EXECL_ARG(8, last_arg, seen_null, args, args__)
        EXECL_ARG(9, last_arg, seen_null, args, args__)
        EXECL_ARG(10, last_arg, seen_null, args, args__)
        va_end(args__);
        char **env = reinterpret_cast<char **>(args[last_arg]);
        args[last_arg] = nullptr;
        return execvpe(__path, args, env);
    })
#elif defined(CAN_WRAP_execle)
#   define WRAPPER_FOR_execle
    FUNCTION_WRAPPER(execle, (int), (char *__path, char *__arg, ...), {
        granary::printf("execle(%s, %s, ...)\n", __path, __arg);
        va_list args__;
        char *args[12] = {nullptr};
        int last_arg(0);
        args[0] = __arg;
        bool seen_null(false);
        va_start(args__, __arg);
        EXECL_ARG(1, last_arg, seen_null, args, args__)
        EXECL_ARG(2, last_arg, seen_null, args, args__)
        EXECL_ARG(3, last_arg, seen_null, args, args__)
        EXECL_ARG(4, last_arg, seen_null, args, args__)
        EXECL_ARG(5, last_arg, seen_null, args, args__)
        EXECL_ARG(6, last_arg, seen_null, args, args__)
        EXECL_ARG(7, last_arg, seen_null, args, args__)
        EXECL_ARG(8, last_arg, seen_null, args, args__)
        EXECL_ARG(9, last_arg, seen_null, args, args__)
        EXECL_ARG(10, last_arg, seen_null, args, args__)
        va_end(args__);
        (void) last_arg;
        return execle(
            __path, args[0], args[1], args[2], args[3], args[4],
            args[5], args[6], args[7], args[8], args[9], args[10]
        );
    })
#endif


#ifdef CAN_WRAP_semctl
#   define WRAPPER_FOR_semctl
    FUNCTION_WRAPPER(semctl, (int), (int _arg1, int _arg2, int _arg3, ...), {
        va_list args__;
        va_start(args__, _arg3);
        union semun _arg4 = va_arg(args__, union semun);
        va_end(args__);
        int ret = semctl(_arg1, _arg2, _arg3, _arg4);
        return ret;
    })
#endif


#define WRAPPER_FOR_open
FUNCTION_WRAPPER(open, (int), (const char *_arg1, int _arg2, ...), {
    va_list args__;
    va_start(args__, _arg2);
    mode_t _arg3 = va_arg(args__, int);
    va_end(args__);
    return open(_arg1, _arg2, _arg3);
})


int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);

#if 0
#define WRAPPER_FOR_pthread_create
FUNCTION_WRAPPER(pthread_create, (int), (pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg), {

})
#endif



#if 0
#ifdef CAN_WRAP_pread
#   define WRAPPER_FOR_pread
    FUNCTION_WRAPPER(pread, (ssize_t), (int fd, void *buf, size_t count, off_t offset), {
        granary::printf("function_wrapper(pread, %d, %p, %lu, %lu)\n", fd, buf, count, offset);
        return pread(fd, buf, count, offset);
    })
#endif
#define WRAPPER_FOR_lseek
FUNCTION_WRAPPER(lseek, (off_t), (int fildes, off_t offset, int whence), {
    granary::printf("function_wrapper(lseek, %d, %lu, %d)\n", fildes, offset, whence);
    return lseek(fildes, offset, whence);
})

#define WRAPPER_FOR_close
FUNCTION_WRAPPER(close, (int), (int fd), {
    int ret(close(fd));
    granary::printf("function_wrapper(close, %d) -> %d\n", fd, ret);
    return ret;
})

#define WRAPPER_FOR_write
FUNCTION_WRAPPER(write, (size_t), (int fildes, const void *buf, size_t nbytes), {
    granary::printf("function_wrapper(write, %d, %p, %lu)\n", fildes, buf, nbytes);
    return write(fildes, buf, nbytes);
})

#define WRAPPER_FOR_sigaction
FUNCTION_WRAPPER(sigaction, (int), (int, const struct sigaction * , struct sigaction * ), {
    return 0;
})

#define WRAPPER_FOR_fopen
FUNCTION_WRAPPER(fopen, (FILE *), (const char *_arg1, const char *_arg2), {
    granary::printf("function_wrapper(fopen, %s, %s)\n", _arg1, _arg2);
    FILE *ret = fopen(_arg1, _arg2);
    RETURN_WRAP(ret);
    return ret;
})

#if 0
#define WRAPPER_FOR_free
FUNCTION_WRAPPER(free, (void), (void *addr), {
    granary::printf("function_wrapper(free, %p)\n", addr);
})
#endif

#define WRAPPER_FOR_getenv
FUNCTION_WRAPPER(getenv, (char *), (const char *arg1), {
    char *ret(getenv(arg1));
    granary::printf("function_wrapper(getenv, %s) -> %s\n", arg1, ret);
    return ret;
})
#endif

#endif /* granary_USER_POSIX_OVERRIDE_WRAPPERS_H_ */
