/*
 * wrappers.h
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_USER_POSIX_OVERRIDE_WRAPPERS_H_
#define granary_USER_POSIX_OVERRIDE_WRAPPERS_H_

#define WRAPPER_FOR_pread
FUNCTION_WRAPPER(pread, (ssize_t), (int fd, void *buf, size_t count, off_t offset), {
    granary::printf("function_wrapper(pread, %d, %p, %lu, %lu)\n", fd, buf, count, offset);
    return pread(fd, buf, count, offset);
})

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

#define WRAPPER_FOR_open
FUNCTION_WRAPPER(open, (int), (const char *_arg1, int _arg2, ...), {
    va_list args__;
    va_start(args__, _arg2);
    // TODO: variadic arguments
    int ret = open(_arg1, _arg2);
    va_end(args__);
    granary::printf("function_wrapper(open, %s, %d) -> %d\n", _arg1, _arg2, ret);
    return ret;
})


#endif /* granary_USER_POSIX_OVERRIDE_WRAPPERS_H_ */
