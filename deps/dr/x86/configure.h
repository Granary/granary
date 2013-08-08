
#ifndef X86_64
#   define X86_64 1
#endif

#ifndef X64
#   define X64 1
#endif

#ifndef LINUX
#   define LINUX 1
#endif

#ifndef UNIX
#	define UNIX 1
#endif

#ifndef LINUX_KERNEL
#   define LINUX_KERNEL 1
#endif

#define GRANARY 1

#ifdef WINDOWS
#   undef WINDOWS
#endif

#define ASSEMBLE_WITH_GAS
