/*
 * globals.h
 *
 * This file is a mishmash of declarations and defines from DynamoRIO. There
 * is enough in here to get a stand-alone instruction encoder/decoder, and not
 * enough more ;-)
 */

#ifndef Granary_GLOBALS_H_
#define Granary_GLOBALS_H_

#define GRANARY
#define FAULT (break_before_fault(), ((*((int *) 0)) = 0))
#define IF_GRANARY(x) x
#define IF_NOT_GRANARY(x)
#define LINUX 1
#define X64 1
#define CLIENT_INTERFACE 1
#define DR_API
#define DR_UNS_API
#define IN
#define INOUT
#define OUT
#define INSTR_INLINE
#define STANDALONE_DECODER 1
#define LOG(...)
#define DODEBUG(...)
#define CLIENT_ASSERT(cond, ...) { if(!(cond)) { FAULT; } }
#define SELF_UNPROTECT_DATASEC(...)
#define SELF_PROTECT_DATASEC(...)
#define IF_X64(x) x
#define IF_X64_ELSE(x, y) x
#define IF_X64_(x) x,
#define _IF_X64(x) , x
#define IF_NOT_X64(x)
#define _IF_NOT_X64(x)
#define IF_DEBUG(x)
#define _IF_DEBUG(x)
#define IF_DEBUG_(x)
#define IF_DEBUG_ELSE(x, y) y
#define IF_DEBUG_ELSE_0(x) 0
#define IF_DEBUG_ELSE_NULL(x) (NULL)
#define DEBUG_DECLARE(...)
#define API_EXPORT_ONLY
#define dr_mcontext_as_priv_mcontext(x) (x)
#define use_addr_prefix_on_short_disp() 0
#define SHARED_FRAGMENTS_ENABLED() 0

#define IF_LINUX(x) x
#define IF_CLIENT_INTERFACE(x)
#define IF_INTERNAL(x)
#define STATS_INC(x)
#define DOLOG(...)
#define HEAPACCT(...)
#define DEBUG_EXT_DECLARE(...) __VA_ARGS__
#define DOCHECK(cond, ...) if(cond) __VA_ARGS__

#define NUM_XMM_SAVED 0

/* check if all bits in mask are set in var */
#define TESTALL(mask, var) (((mask) & (var)) == (mask))
/* check if any bit in mask is set in var */
#define TESTANY(mask, var) (((mask) & (var)) != 0)
/* check if a single bit is set in var */
#define TEST TESTANY

#define BOOLS_MATCH(a, b) (!!(a) == !!(b))

#ifdef LINUX
# define LINK_NI_SYSCALL_ALL (LINK_NI_SYSCALL | LINK_NI_SYSCALL_INT)
#else
# define LINK_NI_SYSCALL_ALL LINK_NI_SYSCALL
#endif

#ifdef DEBUG
#   undef DEBUG
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define NULL ((void *) 0)

#define INTERNAL_OPTION(x) INTERNAL_OPTION_ ## x
#define INTERNAL_OPTION_mangle_app_seg 1

#define DYNAMO_OPTION(x) DYNAMO_OPTION_ ## x
#define DYNAMO_OPTION_shared_trace_ibl_routine 0
#define DYNAMO_OPTION_x86_to_x64 0

#define TLS_XAX_SLOT 0
#define TLS_XBX_SLOT 0
#define TLS_XCX_SLOT 0
#define TLS_XDX_SLOT 0

#define dynamo_exited 0
#define dynamo_initialized 0
#define assert_reachable 0
#define GLOBAL_DCONTEXT (&DCONTEXT)

#define CHECK_TRUNCATE_TYPE_byte(val) ((val) >= 0 && (val) <= UCHAR_MAX)
#define CHECK_TRUNCATE_TYPE_sbyte(val) ((val) <= SCHAR_MAX && ((int64)(val)) >= SCHAR_MIN)
#define CHECK_TRUNCATE_TYPE_ushort(val) ((val) >= 0 && (val) <= USHRT_MAX)
#define CHECK_TRUNCATE_TYPE_short(val) ((val) <= SHRT_MAX && ((int64)(val)) >= SHRT_MIN)
#define CHECK_TRUNCATE_TYPE_uint(val) ((val) >= 0 && (val) <= UINT_MAX)
#if !defined(GRANARY) && defined(LINUX)
/* We can't do the proper int check on Linux because it generates a warning if val has
* type uint that I can't seem to cast around and is impossible to ignore -
* "comparison is always true due to limited range of data type".
* See http://gcc.gnu.org/ml/gcc/2006-01/msg00784.html and note that the suggested
* workaround option doesn't exist as far as I can tell. We are potentially in trouble
* if val has type int64, is negative, and too big to fit in an int. */
# define CHECK_TRUNCATE_TYPE_int(val) ((val) <= INT_MAX)
#else
# define CHECK_TRUNCATE_TYPE_int(val) ((val) <= INT_MAX && ((int64)(val)) >= INT_MIN)
#endif
#ifdef X64
# define CHECK_TRUNCATE_TYPE_size_t(val) ((val) >= 0)
/* avoid gcc warning: always true anyway since stats_int_t == int64 */
# define CHECK_TRUNCATE_TYPE_stats_int_t(val) (true)
#else
# define CHECK_TRUNCATE_TYPE_size_t CHECK_TRUNCATE_TYPE_uint
# define CHECK_TRUNCATE_TYPE_stats_int_t CHECK_TRUNCATE_TYPE_int
#endif


#define notify(...)

#define SYSLOG_COMMON(synch, type, id, sub, ...) \
    notify(type, false, synch, IF_WINDOWS_(MSG_##id) sub, #type, MSG_##id##_STRING, __VA_ARGS__)

#define SYSLOG_INTERNAL_COMMON(synch, type, ...) \
    notify(type, true, synch, IF_WINDOWS_(MSG_INTERNAL_##type) 0, #type, __VA_ARGS__)

/* For security messages use passed in fmt string instead of eventlog fmt
 * string for LOG/stderr/msgbox to avoid breaking our regression suite,
 * NOTE assumes actual id passed, not name of id less MSG_ (so can use array
 * of id's in vmareas.c, another reason need separate fmt string)
 * FIXME : use events.mc string instead (SYSLOG_COMMON), breaks regression
 * This is now used for out-of-memory as well, for the same reason --
 * we should have a mechanism to strip the application name & pid prefix,
 * then we could use the eventlog string.
 */
#define SYSLOG_CUSTOM_NOTIFY(type, id, sub, ...) \
    notify(type, false, true, IF_WINDOWS_(id) sub, #type, __VA_ARGS__)

#define SYSLOG(type, id, sub, ...) \
    SYSLOG_COMMON(true, type, id, sub, __VA_ARGS__)
#define SYSLOG_NO_OPTION_SYNCH(type, id, sub, ...) \
    SYSLOG_COMMON(false, type, id, sub, __VA_ARGS__)

#if defined(INTERNAL) && !defined(STANDALONE_DECODER)
# define SYSLOG_INTERNAL(type, ...) \
      SYSLOG_INTERNAL_COMMON(true, type, __VA_ARGS__)
# define SYSLOG_INTERNAL_NO_OPTION_SYNCH(type, ...) \
      SYSLOG_INTERNAL_COMMON(false, type, __VA_ARGS__)
#else
# define SYSLOG_INTERNAL(...)
# define SYSLOG_INTERNAL_NO_OPTION_SYNCH(...)
#endif /* INTERNAL */

/* for convenience */
#define SYSLOG_INTERNAL_INFO(...) \
    SYSLOG_INTERNAL(SYSLOG_INFORMATION, __VA_ARGS__)
#define SYSLOG_INTERNAL_WARNING(...) \
    SYSLOG_INTERNAL(SYSLOG_WARNING, __VA_ARGS__)
#define SYSLOG_INTERNAL_ERROR(...) \
    SYSLOG_INTERNAL(SYSLOG_ERROR, __VA_ARGS__)
#define SYSLOG_INTERNAL_CRITICAL(...) \
    SYSLOG_INTERNAL(SYSLOG_CRITICAL, __VA_ARGS__)

#define SYSLOG_INTERNAL_INFO_ONCE(...) \
    DODEBUG_ONCE(SYSLOG_INTERNAL_INFO(__VA_ARGS__))
#define SYSLOG_INTERNAL_WARNING_ONCE(...) \
    DODEBUG_ONCE(SYSLOG_INTERNAL_WARNING(__VA_ARGS__))
#define SYSLOG_INTERNAL_ERROR_ONCE(...) \
    DODEBUG_ONCE(SYSLOG_INTERNAL_ERROR(__VA_ARGS__))
#define SYSLOG_INTERNAL_CRITICAL_ONCE(...) \
    DODEBUG_ONCE(SYSLOG_INTERNAL_CRITICAL(__VA_ARGS__))

#define synchronize_dynamic_options()
#define os_terminate(...) CLIENT_ASSERT(false, void)
#define os_dump_core(...) CLIENT_ASSERT(false, void)
#define DUMPCORE_FATAL_USAGE_ERROR 0
#define DYNAMO_OPTION_NOT_STRING(opt) 0
#define IF_HAVE_TLS_ELSE(if_true, if_false) (if_false)

/* FIXME, eventually want usage_error to also be external (may also eventually
 * need non dynamic option synch form as well for usage errors while updating
 * dynamic options), but lot of work to get all in eventlog and currently only
 * really triggered by internal options */
/* FIXME : could leave out the asserts, this is a recoverable error */
#define USAGE_ERROR(...)                                                     \
    do {                                                                     \
        SYSLOG_INTERNAL_ERROR(__VA_ARGS__);                                  \
        ASSERT_NOT_REACHED();                                                \
    } while (0)
#define FATAL_USAGE_ERROR(id, sub, ...)                                      \
    do {                                                                     \
        /* synchronize dynamic options for dumpcore_mask */                  \
        synchronize_dynamic_options();                                       \
        if (TEST(DUMPCORE_FATAL_USAGE_ERROR, DYNAMO_OPTION_NOT_STRING(dumpcore_mask)))  \
            os_dump_core("fatal usage error");                               \
        SYSLOG(SYSLOG_CRITICAL, id, sub,  __VA_ARGS__);                      \
        os_terminate(NULL, TERMINATE_PROCESS);                               \
    } while (0)
#define OPTION_PARSE_ERROR(id, sub, ...)                                     \
    do {                                                                     \
        SYSLOG_NO_OPTION_SYNCH(SYSLOG_ERROR, id, sub, __VA_ARGS__);          \
        DODEBUG(os_terminate(NULL, TERMINATE_PROCESS););                     \
    } while (0)

#ifdef DEBUG
/* only for temporary tracing - do not leave in source, use make ugly to remind you */
# define TRACELOG(level) LOG(GLOBAL, LOG_TOP, level, "%s:%d ", __FILE__, __LINE__)
#else
# define TRACELOG(level)
#endif

#define REL32_REACHABLE_OFFS(offs) ((offs) <= INT_MAX && (offs) >= INT_MIN)
/* source should be the end of a rip-relative-referencing instr */
#define REL32_REACHABLE(source, target) REL32_REACHABLE_OFFS((target) - (source))



/* Reachability helpers */
/* Given a region, returns the start of the enclosing region that can reached by a
 * 32bit displacement from everywhere in the supplied region. Checks for underflow. If the
 * supplied region is too large then returned value may be > reachable_region_start
 * (caller should check) as the constraint may not be satisfiable. */
/* i#14: gcc 4.3.0 treats "ptr - const < ptr" as always true,
 * so we work around that here.  could adapt POINTER_OVERFLOW_ON_ADD
 * or cast to ptr_uint_t like it does, instead.
 */
# define REACHABLE_32BIT_START(reachable_region_start, reachable_region_end) \
    (((reachable_region_end) > ((byte *)(ptr_uint_t)(uint)(INT_MIN))) ?      \
     (reachable_region_end) + INT_MIN : (byte *)PTR_UINT_0)
/* Given a region, returns the end of the enclosing region that can reached by a
 * 32bit displacement from everywhere in the supplied region. Checks for overflow. If the
 * supplied region is too large then returned value may be < reachable_region_end
 * (caller should check) as the constraint may not be satisfiable. */
# define REACHABLE_32BIT_END(reachable_region_start, reachable_region_end)   \
    (((reachable_region_start) < ((byte *)POINTER_MAX) - INT_MAX) ?          \
     (reachable_region_start) + INT_MAX : (byte *)POINTER_MAX)

#define MAX_LOW_2GB ((byte*)(ptr_uint_t)INT_MAX)

/* alignment helpers, alignment must be power of 2 */
#define ALIGNED(x, alignment) ((((ptr_uint_t)x) & ((alignment)-1)) == 0)
#define ALIGN_FORWARD(x, alignment) \
    ((((ptr_uint_t)x) + ((alignment)-1)) & (~((ptr_uint_t)(alignment)-1)))
#define ALIGN_FORWARD_UINT(x, alignment) \
    ((((uint)x) + ((alignment)-1)) & (~((alignment)-1)))
#define ALIGN_BACKWARD(x, alignment) (((ptr_uint_t)x) & (~((ptr_uint_t)(alignment)-1)))
#define PAD(length, alignment) (ALIGN_FORWARD((length), (alignment)) - (length))
#define ALIGN_MOD(addr, size, alignment) \
    ((((ptr_uint_t)addr)+(size)-1) & ((alignment)-1))
#define CROSSES_ALIGNMENT(addr, size, alignment) \
    (ALIGN_MOD(addr, size, alignment) < (size)-1)
/* number of bytes you need to shift addr forward so that it's !CROSSES_ALIGNMENT */
#define ALIGN_SHIFT_SIZE(addr, size, alignment) \
    (CROSSES_ALIGNMENT(addr, size, alignment) ?   \
        ((size) - 1 - ALIGN_MOD(addr, size, alignment)) : 0)

#define IS_POWER_OF_2(x) ((x) == 0 || ((x) & ((x)-1)) == 0)

/* C standard has pointer overflow as undefined so cast to unsigned
 * (i#14 and drmem i#302)
 */
#define POINTER_OVERFLOW_ON_ADD(ptr, add) \
    (((ptr_uint_t)(ptr)) + (add) < ((ptr_uint_t)(ptr)))
#define POINTER_UNDERFLOW_ON_SUB(ptr, sub) \
    (((ptr_uint_t)(ptr)) - (sub) > ((ptr_uint_t)(ptr)))

#ifdef DEBUG
#else
# define ASSERT(x)         ((void) 0)
# define ASSERT_MESSAGE(level, msg, x) ASSERT(x)
# define ASSERT_NOT_TESTED() ASSERT(true)
# define ASSERT_CURIOSITY(x) ASSERT(true)
# define ASSERT_CURIOSITY_ONCE(x) ASSERT(true)
#endif /* DEBUG */

#define ASSERT_NOT_REACHED() ASSERT(false)
#define ASSERT_BUG_NUM(num, x) ASSERT_MESSAGE(CHKLVL_ASSERTS, "Bug #"#num, x)
#define ASSERT_NOT_IMPLEMENTED(x) ASSERT_MESSAGE(CHKLVL_ASSERTS, "Not implemented", x)
#define EXEMPT_TEST(tests) check_filter(tests, get_short_name(get_application_name()))

/* var = (type) val; should always be preceded by a call to ASSERT_TRUNCATE  */
/* note that it is also OK to use ASSERT_TRUNCATE(type, type, val) for return values */
#define ASSERT_TRUNCATE(var, type, val) ASSERT(sizeof(type) == sizeof(var) && \
                                               CHECK_TRUNCATE_TYPE_##type(val) && \
                                               "truncating "#var" to "#type)
#define CURIOSITY_TRUNCATE(var, type, val) \
    ASSERT_CURIOSITY(sizeof(type) == sizeof(var) && \
                                               CHECK_TRUNCATE_TYPE_##type(val) && \
                                               "truncating "#var" to "#type)
#define CLIENT_ASSERT_TRUNCATE(var, type, val, msg) \
    CLIENT_ASSERT(sizeof(type) == sizeof(var) && CHECK_TRUNCATE_TYPE_##type(val), msg)

/* assumes val is unsigned and width<32 */
#define ASSERT_BITFIELD_TRUNCATE(width, val) \
    ASSERT((val) < (1 << ((width)+1)) && "truncating to "#width" bits");
#define CLIENT_ASSERT_BITFIELD_TRUNCATE(width, val, msg) \
    CLIENT_ASSERT((val) < (1 << ((width)+1)), msg);



#define UNPROTECTED_LOCAL_ALLOC(dc, ...) global_unprotected_heap_alloc(__VA_ARGS__)
#define UNPROTECTED_LOCAL_FREE(dc, ...) global_unprotected_heap_free(__VA_ARGS__)
#define UNPROTECTED_GLOBAL_ALLOC global_unprotected_heap_alloc
#define UNPROTECTED_GLOBAL_FREE global_unprotected_heap_free

#define PROTECTED true

// MWAHAHAHAH
#define UNPROTECTED true
#define global_unprotected_heap_alloc(...) NULL

#define HEAP_ARRAY_ALLOC(dc, type, num, which, protected) \
    ((protected) ? \
        (type *) heap_alloc(dc, sizeof(type)*(num) HEAPACCT(which)) : \
        (type *) UNPROTECTED_LOCAL_ALLOC(dc, sizeof(type)*(num) HEAPACCT(which)))
#define HEAP_TYPE_ALLOC(dc, type, which, protected) \
    HEAP_ARRAY_ALLOC(dc, type, 1, which, protected)
#define HEAP_ARRAY_ALLOC_MEMSET(dc, type, num, which, protected, val)   \
    memset(HEAP_ARRAY_ALLOC(dc, type, num, which, protected), (val),    \
           sizeof(type)*(num));

#define HEAP_ARRAY_FREE(dc, p, type, num, which, protected) \
    ((protected) ? \
        heap_free(dc, (type*)p, sizeof(type)*(num) HEAPACCT(which)) : \
        UNPROTECTED_LOCAL_FREE(dc, (type*)p, sizeof(type)*(num) HEAPACCT(which)))
#define HEAP_TYPE_FREE(dc, p, type, which, protected) \
    HEAP_ARRAY_FREE(dc, p, type, 1, which, protected)

#include "dr/types.h"
#include "dr/x86/proc.h"
#include "dr/link.h"
#include "dr/instrlist.h"

extern void *heap_alloc(void *, unsigned long long);
extern void heap_free(void *, void *, unsigned long long);
extern dcontext_t *get_thread_private_dcontext(void);

extern void break_before_fault(void);

#ifdef __cplusplus
}
#endif

#endif /* Granary_GLOBALS_H_ */
