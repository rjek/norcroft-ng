/* stddef.h: ISO 'C' library header, section 7.19, for x86-64 Linux.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * glibc's headers include this file with one or more of __need_size_t,
 * __need_ptrdiff_t, __need_wchar_t, __need_wint_t and __need_NULL
 * defined when they only want some of its definitions.
 */

#if !defined __need_size_t && !defined __need_ptrdiff_t && \
    !defined __need_wchar_t && !defined __need_wint_t && \
    !defined __need_NULL
#  define __stddef_want_all
#endif

#if defined __stddef_want_all || defined __need_ptrdiff_t
#  ifndef _PTRDIFF_T
#    define _PTRDIFF_T
typedef long ptrdiff_t;
#  endif
#endif

#if defined __stddef_want_all || defined __need_size_t
#  ifndef __size_t
#    define __size_t 1
#    define _SIZE_T
typedef unsigned long size_t;
#  endif
#endif

#if defined __stddef_want_all || defined __need_wchar_t
#  ifndef __wchar_t
#    define __wchar_t 1
#    define _WCHAR_T
typedef int wchar_t;
#  endif
#endif

#if defined __need_wint_t
#  ifndef _WINT_T
#    define _WINT_T
typedef unsigned int wint_t;
#  endif
#endif

#if defined __stddef_want_all || defined __need_NULL
#  undef NULL
#  define NULL ((void *)0)
#endif

#if defined __stddef_want_all && !defined offsetof
#  define offsetof(type, member) \
      ((size_t)((char *)&(((type *)0)->member) - (char *)0))
#endif

#if defined __stddef_want_all && defined __STDC_VERSION__ && \
    __STDC_VERSION__ >= 201112L && !defined __max_align_t_defined
#  define __max_align_t_defined
/* (As gcc's, so as malloc() aligns.)                                   */
typedef struct {
    _Alignas(16) long long __max_align_ll;
    long double __max_align_ld;
} max_align_t;
#endif

#if defined __stddef_want_all && defined __STDC_VERSION__ && \
    __STDC_VERSION__ >= 202311L && !defined __nullptr_t_defined
#  define __nullptr_t_defined
typedef typeof(nullptr) nullptr_t;
#  define unreachable() ((void)0)
#  define __STDC_VERSION_STDDEF_H__ 202311L
#endif

#undef __stddef_want_all
#undef __need_ptrdiff_t
#undef __need_size_t
#undef __need_wchar_t
#undef __need_wint_t
#undef __need_NULL

/* end of stddef.h */
