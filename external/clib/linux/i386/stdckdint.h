/* stdckdint.h: ISO 'C' library header, section 7.20 (C23 numbering).
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * ckd_add(r, a, b), ckd_sub(r, a, b) and ckd_mul(r, a, b) compute a+b,
 * a-b and a*b exactly, store the result in *r wrapped to the type of *r,
 * and return whether it didn't fit.  The operands and *r may be of any
 * integer types (including _BitInt) of up to 64 bits.
 *
 * The exact result is kept as a sign and a 64-bit magnitude, noting if
 * the magnitude needs more bits.  Whether it fits is found by comparing
 * it with the value that was stored.  Each of r, a and b is evaluated
 * once (typeof doesn't evaluate its operand).
 */

#ifndef __stdckdint_h
#define __stdckdint_h

#define __STDC_VERSION_STDCKDINT_H__ 202311L

typedef struct { int __neg, __big; unsigned long long __mag; } __ckd_t;

/* The value whose bits (converted to unsigned long long) are b, of a   */
/* signed type if s.                                                    */
static __inline __ckd_t __ckd_mk(unsigned long long __b, int __s)
{   __ckd_t __r;
    unsigned long long __top = __b >> 63;
    __r.__neg = __s && __top != 0;
    __r.__big = 0;
    __r.__mag = __r.__neg ? 0 - __b : __b;
    return __r;
}

/* x as a __ckd_t: (T)-1 < 1 exactly if the type T is signed.            */
#define __ckd_v(x) __ckd_mk((unsigned long long)(x), (__typeof__(x))-1 < 1)

/* (Each unsigned long long result below is stored in a variable before */
/* being shifted or compared, so that it is treated as unsigned even    */
/* by targets which do long long arithmetic by calling functions.)      */

static __inline __ckd_t __ckd_add(__ckd_t __x, __ckd_t __y)
{   __ckd_t __r;
    unsigned long long __m;
    __r.__big = 0;
    if (__x.__neg == __y.__neg)
    {   __m = __x.__mag + __y.__mag;
        __r.__neg = __x.__neg;
        __r.__big = __m < __x.__mag;
    }
    else if (__x.__mag >= __y.__mag)
    {   __m = __x.__mag - __y.__mag;
        __r.__neg = __x.__neg;
    }
    else
    {   __m = __y.__mag - __x.__mag;
        __r.__neg = __y.__neg;
    }
    __r.__mag = __m;
    if (__m == 0 && !__r.__big) __r.__neg = 0;
    return __r;
}

static __inline __ckd_t __ckd_sub(__ckd_t __x, __ckd_t __y)
{   __y.__neg = !__y.__neg;
    return __ckd_add(__x, __y);
}

static __inline __ckd_t __ckd_mul(__ckd_t __x, __ckd_t __y)
{   __ckd_t __r;
    /* The 128-bit product of the magnitudes, from 32-bit halves.       */
    unsigned long long __al = __x.__mag & 0xffffffffu, __ah = __x.__mag >> 32;
    unsigned long long __bl = __y.__mag & 0xffffffffu, __bh = __y.__mag >> 32;
    unsigned long long __ll = __al * __bl, __lh = __al * __bh;
    unsigned long long __hl = __ah * __bl, __hh = __ah * __bh;
    unsigned long long __c = __ll >> 32;
    unsigned long long __mid = __c + (__lh & 0xffffffffu) + (__hl & 0xffffffffu);
    unsigned long long __lo = (__ll & 0xffffffffu) | (__mid << 32);
    unsigned long long __c1 = __lh >> 32, __c2 = __hl >> 32, __c3 = __mid >> 32;
    unsigned long long __hi = __hh + __c1 + __c2 + __c3;
    __r.__neg = __x.__neg != __y.__neg;
    __r.__big = __hi != 0;
    __r.__mag = __lo;
    if (__lo == 0 && __hi == 0) __r.__neg = 0;
    return __r;
}

/* The low 64 bits of x, in two's complement.                           */
static __inline unsigned long long __ckd_bits(__ckd_t __x)
{   unsigned long long __m = __x.__mag;
    return __x.__neg ? 0 - __m : __m;
}

/* Whether want (the exact result) differs from got (what was stored).  */
static __inline _Bool __ckd_ovf(__ckd_t const *__want, __ckd_t __got)
{   return __want->__big || __want->__neg != __got.__neg ||
           __want->__mag != __got.__mag;
}

/* Somewhere to keep the exact result while *r is assigned.            */
#if defined __x86_64__ && __STDC_VERSION__ >= 201112L    /* (has TLS) */
#  define __ckd_local static _Thread_local
#else
#  define __ckd_local static
#endif
static __inline __ckd_t *__ckd_tmp(void)
{   __ckd_local __ckd_t __t;
    return &__t;
}

#define __ckd_op(f, r, a, b) \
    (*__ckd_tmp() = f(__ckd_v(a), __ckd_v(b)), \
     __ckd_ovf(__ckd_tmp(), \
               __ckd_v(*(r) = (__typeof__(*(r)))__ckd_bits(*__ckd_tmp()))))

#define ckd_add(r, a, b) __ckd_op(__ckd_add, r, a, b)
#define ckd_sub(r, a, b) __ckd_op(__ckd_sub, r, a, b)
#define ckd_mul(r, a, b) __ckd_op(__ckd_mul, r, a, b)

#endif

/* end of stdckdint.h */
