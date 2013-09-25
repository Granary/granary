/**
 * Part of the DynamoRIO Kernel (DRK) project at the University of Toronto.
 * (c) 2010 Peter Feiner.
 */

#ifndef MSR_H_
#define MSR_H_

#include <stdint.h>

enum {
    /* Copied from linux-2.6.32/arch/x86/include/asm/msr-index.h */
    MSR_EFER = 0xc0000080, /* extended feature register */
    MSR_STAR = 0xc0000081, /* legacy mode SYSCALL target */
    MSR_LSTAR = 0xc0000082, /* long mode SYSCALL target */
    MSR_CSTAR = 0xc0000083, /* compat mode SYSCALL target */
    MSR_SYSCALL_MASK = 0xc0000084, /* EFLAGS mask for syscall */
    MSR_FS_BASE = 0xc0000100, /* 64bit FS base */
    MSR_GS_BASE = 0xc0000101, /* 64bit GS base */
    MSR_KERNEL_GS_BASE = 0xc0000102, /* SwapGS GS shadow */
    MSR_IA32_SYSENTER_CS = 0x00000174,
    MSR_IA32_SYSENTER_ESP = 0x00000175,
    MSR_IA32_SYSENTER_EIP = 0x00000176,
};

static inline uint64_t
get_msr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;
    /* Copied from arch/x86/include/asm/msr.h */
    asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return (((uint64_t) high) << 32) | low;
}

static inline void
set_msr(uint32_t msr, uint64_t value)
{
    uint32_t low = value & 0xffffffff;
    uint32_t high = (value >> 32) & 0xffffffff;
    /* Copied from arch/x86/include/asm/msr.h */
    asm volatile("wrmsr" : : "c" (msr), "a"(low), "d" (high) : "memory");
}


#endif /* MSR_H_ */
