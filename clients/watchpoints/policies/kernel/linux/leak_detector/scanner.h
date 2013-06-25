/*
 * scanner.h
 *
 *  Created on: 2013-06-24
 *      Author: akshayk
 */

#ifndef _LEAK_POLICY_SCANNER_H_
#define _LEAK_POLICY_SCANNER_H_

#if defined(__GNUC__) && !defined(__clang__)
#   define DECLTYPE decltype
#   define OFFSETOF __builtin_offsetof
#else
#   define DECLTYPE decltype
#   define OFFSETOF offsetof
#endif

#ifndef NULL
#   define NULL 0
#endif

#define IS_VALID_ADDRESS(x)

/* tag type for the scanner functions*/
class pre_tag { };
class post_tag { };

template <typename T>
class static_tag {
public:
    static const T tag;
};

template <typename T>
const T static_tag<T>::tag;

/// dummy for returning the same type; guarantees typdef syntax correctness
template <typename T>
class identity_type {
public:
    typedef T type;
};

#define PRE_SCAN \
    FORCE_INLINE static void scan(ArgT__ &arg, const int depth__)



#define TYPE_SCAN_WRAPPER(type_name, body) \
    template <> \
    class scan_function<type_name> { \
    public: \
        enum { \
            NUM = 1, \
            IS_DEFINED = 1 \
        }; \
        typedef identity_type<type_name>::type ArgT__; \
        static unsigned id_;\
        struct scan_impl body ; \
    };


#define SCAN_FUNCTION(lval) \
            type_scanner_field(lval); \


#define SCAN_HEAD   \
        FORCE_INLINE static void scan_head(ArgT__ &arg)


#define SCAN_RECURSIVE(lval)  \
            if(depth__ <= MAX_DEPTH_SCANNER)    {   \
                scan_recursive<scan_function, DECLTYPE(lval)>(lval, depth__+1);\
            }


#define SCAN_RECURSIVE_PTR(lval)

#define MAX_DEPTH_SCANNER 5

#define SCANNER(type_name) \
        scan_function<type_name>::scan_impl::scan(arg, MAX_DEPTH_SCANNER)

#define SCAN_HEAD_FUNC(type_name) \
        scan_function<type_name>::scan_impl::scan_head


template <typename T>
class scan_function {
public:
    enum {
        NUM = 0,
        IS_DEFINED = 1
    };

    struct scan_impl {
        FORCE_INLINE static void scan_head(T &) { }
        FORCE_INLINE static void scan(T & , const int depth__) { }
    };
};

template <>
class scan_function<void *> {
public:
    enum {
        NUM = 0,
        IS_DEFINED = 1
    };
    static unsigned id_;
    struct scan_impl{
        FORCE_INLINE static void scan_head(void*) { }
        FORCE_INLINE static void scan(void*, const int depth__) { }
    };
};


/// type reduction for recursive scanning
template <template<typename> class Scan, typename Arg>
class recursive_scanner_base {
public:
    enum {
        NUM = Scan<Arg>::NUM
    };

    inline static void pre_scan(Arg &arg, const int depth__) {
        Scan<Arg>::scan_impl::scan(arg, depth__);
    }
};

/// type reduction for recursive scanning
template <template<typename> class Scan, typename Arg>
class recursive_scanner : public recursive_scanner_base<Scan, Arg> { };

template <template<typename> class Scan, typename Arg>
class recursive_scanner<Scan, const Arg> {
public:

    enum {
        NUM = Scan<Arg>::NUM
    };

    inline static void pre_scan(const Arg &arg, const int depth__) {
        recursive_scanner<Scan, Arg>::pre_scan(const_cast<Arg &>(arg), depth__);
    }
};

template <template<typename> class Scan, typename Arg>
class recursive_scanner<Scan, Arg *> {
public:

    enum {
        NUM = Scan<Arg *>::IS_DEFINED
            ? 1
            : recursive_scanner<Scan, Arg>::NUM
    };

    inline static void pre_scan(Arg *&arg, const int depth__) {
        if(((Arg *) 4095) < arg) {
            if(Scan<Arg *>::IS_DEFINED) {
                recursive_scanner_base<Scan, Arg *>::pre_scan(arg, depth__);
            } else {
                recursive_scanner<Scan, Arg>::pre_scan(*arg, depth__);
            }
        }
    }
};

template <template<typename> class Scan, typename Arg>
class recursive_scanner<Scan, Arg **> {
public:

    enum {
        NUM = recursive_scanner<Scan, Arg *>::NUM
    };

    inline static void pre_scan(Arg **&arg, const int depth__) {
        if(((Arg **) 4095) < arg) {
            recursive_scanner<Scan, Arg *>::pre_scan(*arg, depth__);
        }
    }
};

template <template<typename> class Scan, typename Ret, typename... Args>
class recursive_scanner<Scan, Ret (*)(Args...)> {
public:

    typedef Ret (Arg)(Args...);

    enum {
        NUM = Scan<Arg *>::NUM
    };

    inline static void pre_scan(Arg *&arg, const int depth__) {
        if(((Arg *) 4095) < arg) {
            pre_tag tag;
            Scan<Arg *>::scan_impl::scan(arg, depth__);
        }
    }
};

template <template<typename> class Scan, typename Arg>
class recursive_scanner<Scan, Arg &> : public recursive_scanner<Scan, Arg> { };

template <template<typename> class Scan>
class recursive_scanner<Scan, void *> {
public:
    enum {
        NUM = 0
    };

    FORCE_INLINE static void pre_scan(void *, const int) { }
};

template <template<typename> class Scan>
class recursive_scanner<Scan, const void *> {
public:
    enum {
        NUM = 0
    };

    FORCE_INLINE static void pre_scan(const void *, const int) { }
};

template <template<typename> class Scan, typename Arg>
void scan_recursive(Arg &arg, const int depth__) {
    recursive_scanner<Scan, Arg>::pre_scan(arg, depth__);
}


template <typename T>
bool type_scanner_field(T* ptr) {
    return true;
}

template <typename T>
bool type_scanner_field(T addr) {
    return true;
}

#include "clients/gen/kernel_type_scanners.h"


#endif /* SCANNER_H_ */
