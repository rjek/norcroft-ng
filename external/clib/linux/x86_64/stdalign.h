/* stdalign.h: ISO 'C' library header, section 7.15 (C11).
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * alignas and alignof are keywords in C23.
 */

#ifndef __stdalign_h
#define __stdalign_h

#if !defined __STDC_VERSION__ || __STDC_VERSION__ < 202311L
#  define alignas _Alignas
#  define alignof _Alignof
#  define __alignas_is_defined 1
#  define __alignof_is_defined 1
#endif

#endif

/* end of stdalign.h */
