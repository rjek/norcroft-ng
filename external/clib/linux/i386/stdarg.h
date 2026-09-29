/* stdarg.h: ISO 'C' library header, section 7.16, for i386 Linux.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * On i386 every argument is passed on the stack in 4-byte aligned
 * slots, so a va_list is simply a pointer to the next argument.
 *
 * glibc's headers include this file with __need___va_list defined when
 * they only want the __gnuc_va_list type.
 */

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef char *__gnuc_va_list;
#endif

#ifdef __need___va_list
#undef __need___va_list
#else

#ifndef __stdarg_h
#define __stdarg_h

typedef __gnuc_va_list va_list;

#define __va_size(type) ((sizeof(type) + 3) & ~3)

#define va_start(ap, parmN) \
   ((void)((ap) = (char *)&(parmN) + __va_size(parmN)))
#define va_arg(ap, type) \
   (*(type *)(((ap) += __va_size(type)) - __va_size(type)))
#define va_end(ap) ((void)((ap) = (char *)0))
#define va_copy(dest, src) ((void)((dest) = (src)))
#define __va_copy(dest, src) va_copy(dest, src)

#endif
#endif

/* end of stdarg.h */
