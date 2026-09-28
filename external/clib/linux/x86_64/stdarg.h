/* stdarg.h: ISO 'C' library header, section 7.16, for x86-64 Linux.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * The System V x86-64 ABI passes the first arguments in registers, so a
 * va_list describes both the area the called function saves them to and
 * the arguments on the stack.  __builtin_va_start (done by the compiler)
 * fills it in, and va_arg uses ___typeof to tell floating arguments,
 * which are passed in xmm registers, from integer ones.
 *
 * glibc's headers include this file with __need___va_list defined when
 * they only want the __gnuc_va_list type.
 */

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef struct {
    unsigned int __gp_offset;       /* next integer register in __reg_save_area */
    unsigned int __fp_offset;       /* next xmm register in __reg_save_area     */
    char *__overflow_arg_area;      /* next argument on the stack               */
    char *__reg_save_area;          /* rdi..r9 (48 bytes), then xmm0-7          */
} __va_list_tag;
typedef __va_list_tag __gnuc_va_list[1];
#endif

#ifdef __need___va_list
#undef __need___va_list
#else

#ifndef __stdarg_h
#define __stdarg_h

typedef __gnuc_va_list va_list;

extern void __builtin_va_start(__va_list_tag *);

#define va_start(ap, parmN) __builtin_va_start(ap)

/* The size of an argument in 8-byte words, and whether it is floating   */
/* (see codeoftype() in the compiler).                                   */
#define __va_words(type) ((sizeof(___type type) + 7) / 8)
#define __va_isfp(type)  (___typeof(___type type) & 0x2)

#define __va_stack(ap, type) \
   (((ap)->__overflow_arg_area += 8*__va_words(type)) - 8*__va_words(type))

/* Structs of up to 16 bytes are passed in integer registers if there   */
/* are enough of them (structs of floating point values are not yet     */
/* handled), and larger ones on the stack.                              */
#define va_arg(ap, type) \
   (*(___type type *)( \
      __va_isfp(type) ? \
         ((ap)->__fp_offset < 176 ? \
            ((ap)->__fp_offset += 16, \
             (ap)->__reg_save_area + (ap)->__fp_offset - 16) : \
            __va_stack(ap, type)) : \
      __va_words(type) <= 2 && \
      (ap)->__gp_offset + 8*__va_words(type) <= 48 ? \
         ((ap)->__gp_offset += 8*__va_words(type), \
          (ap)->__reg_save_area + (ap)->__gp_offset - 8*__va_words(type)) : \
      __va_stack(ap, type)))

#define va_end(ap) ((void)0)
#define va_copy(dest, src) ((void)(*(dest) = *(src)))
#define __va_copy(dest, src) va_copy(dest, src)

#endif
#endif

/* end of stdarg.h */
