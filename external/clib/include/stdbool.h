/* stdbool.h: ISO 'C' library header, section 7.18 (C11 numbering).
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * _Bool is a keyword in every C mode (bool, true and false themselves
 * are from C23).
 */

#ifndef __stdbool_h
#define __stdbool_h

#if !defined __STDC_VERSION__ || __STDC_VERSION__ < 202311L
#  define bool  _Bool
#  define true  1
#  define false 0
#endif
#define __bool_true_false_are_defined 1

#endif

/* end of stdbool.h */
