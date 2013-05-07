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
#define WRAPPER_FOR_struct___sFILE
#define WRAPPER_FOR_struct_malloc_introspection_t
#define WRAPPER_FOR_struct__malloc_zone_t
#define WRAPPER_FOR_glob_t


/// Disable wrapping of some Linux types.
#define WRAPPER_FOR_glob64_t
#define WRAPPER_FOR__IO_cookie_io_functions_t

#define EXECL_ARG(num, last_arg, seen_null, args_arr, args_list) \
    (args_arr)[num] = va_arg((args_list), char *); \
    if(!(seen_null)) { \
        (last_arg) = (num); \
        (seen_null) = (nullptr == (args_arr)[num]); \
    } else { \
        (args_arr)[num] = nullptr; \
    }


#if defined(CAN_WRAP_execl) && defined(CAN_WRAP_execv) && CAN_WRAP_execl && !defined(WRAPPER_FOR_execl)
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


#if defined(CAN_WRAP_execlp) && defined(CAN_WRAP_execvp) && CAN_WRAP_execlp && !defined(WRAPPER_FOR_execlp)
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

#if defined(CAN_WRAP_execvp) && CAN_WRAP_execvp && !defined(WRAPPER_FOR_execvp)
#   define WRAPPER_FOR_execvp
    FUNCTION_WRAPPER(execvp, (int), (const char *file, char * const *argv), {
        granary::printf("execvp(%s, ...)\n", file);
        return execvp(file, argv);
    })
#endif


#if defined(CAN_WRAP_execle) && defined(CAN_WRAP_execvpe) && CAN_WRAP_execle && !defined(WRAPPER_FOR_execle)
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
        char **env = reinterpret_cast<char **>(args[last_arg]);
        args[last_arg] = nullptr;
        return execvpe(__path, args, env);
    })
#elif defined(CAN_WRAP_execle) && !defined(WRAPPER_FOR_execle)
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

        const char **envp(unsafe_cast<const char **>(args[last_arg + 1]));

        switch(last_arg) {
        case 0: FAULT;
        case 1: return execle(__path, args[0], nullptr, envp);
        case 2: return execle(__path, args[0], args[1], nullptr, envp);
        case 3: return execle(__path, args[0], args[1], args[2], nullptr, envp);
        case 4:
            return execle(
                __path, args[0], args[1], args[2], args[3], nullptr, envp);
        case 5:
            return execle(
                __path, args[0], args[1], args[2], args[3],
                args[4], nullptr, envp);
        case 6:
            return execle(
                __path, args[0], args[1], args[2], args[3],
                args[4], args[5], nullptr, envp);
        case 7:
            return execle(
                __path, args[0], args[1], args[2], args[3],
                args[4], args[5], args[6], nullptr, envp);
        case 8:
            return execle(
                __path, args[0], args[1], args[2], args[3],
                args[4], args[5], args[6], args[7], nullptr, envp);
        case 9:
            return execle(
                __path, args[0], args[1], args[2], args[3],
                args[4], args[5], args[6], args[7], args[8], nullptr, envp);
        default:
            FAULT;
        }

        return -1;
    })
#endif


#if defined(CAN_WRAP_semctl) && CAN_WRAP_semctl && !defined(WRAPPER_FOR_semctl)
#   define WRAPPER_FOR_semctl
    FUNCTION_WRAPPER(semctl, (int), (int _arg1, int _arg2, int _arg3, ...), {
        va_list args__;
        va_start(args__, _arg3);
        uint64_t _arg4 = va_arg(args__, uint64_t); // union semun
        va_end(args__);
        return semctl(_arg1, _arg2, _arg3, _arg4);
    })
#endif


#if defined(CAN_WRAP_open) && CAN_WRAP_open && !defined(WRAPPER_FOR_open)
#   define WRAPPER_FOR_open
    FUNCTION_WRAPPER(open, (int), (const char *_arg1, int _arg2, ...), {
        va_list args__;
        va_start(args__, _arg2);
        mode_t _arg3 = va_arg(args__, int);
        va_end(args__);
        return open(_arg1, _arg2, _arg3);
    })
#endif


#if defined(CAN_WRAP_vfork) && CAN_WRAP_vfork && !defined(WRAPPER_FOR_vfork)
#   define WRAPPER_FOR_vfork
    FUNCTION_WRAPPER(vfork, (pid_t), (void), {
        return fork();
    })
#endif


#if defined(CAN_WRAP_getenv) && CAN_WRAP_getenv && !defined(WRAPPER_FOR_getenv)
#   define WRAPPER_FOR_getenv
    FUNCTION_WRAPPER(getenv, (char *), (const char *arg1), {

        // For sort-of transparency.
        if(0 == strcmp(arg1, "DYLD_INSERT_LIBRARIES")
        || 0 == strcmp(arg1, "LD_PRELOAD")) {
            return 0;
        } else {
            return getenv(arg1);
        }
    })
#endif


#if defined(CAN_WRAP_dlsym) && CAN_WRAP_dlsym && !defined(WRAPPER_FOR_dlsym)
#   define WRAPPER_FOR_dlsym
    FUNCTION_WRAPPER(dlsym, (void *), (void *handle, const char *sym), {
        return dlsym(handle, sym);
    })
#endif


#endif /* granary_USER_POSIX_OVERRIDE_WRAPPERS_H_ */
