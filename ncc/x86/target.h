/*
 * x86/target.h -- target description for the x86 back end.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Currently only 32-bit i386 (System V i386 psABI, ELF via the system
 * assembler) is supported.  Floating point is done in SSE2 registers.
 */

#ifndef _target_LOADED
#define _target_LOADED 1

#include "toolenv.h"

#define TARGET_IS_X86                   1
#define TARGET_IS_I386                  1

#define TARGET_MACHINE                  "i386"
#define TARGET_PREDEFINES               { "__i386", "__i386__", "i386", \
                                          "__ELF__", "__CHAR_BIT__=8", \
                                          "__SIZEOF_POINTER__=4", \
                                          "__SIZEOF_LONG__=4", \
                                          "__SIZEOF_LONG_LONG__=8", \
                                          "__ORDER_LITTLE_ENDIAN__=1234", \
                                          "__BYTE_ORDER__=1234" }

#define TARGET_IS_LITTLE_ENDIAN         1
#define TARGET_HAS_IEEE                 1
#define TARGET_HAS_BSS                  1
#define CONST_DATA_IN_CODE              1   /* separate area: .rodata  */

#define TARGET_HAS_2ADDRESS_CODE        1
#define TARGET_HAS_MULTIPLY             1
#define TARGET_HAS_DIVIDE               1
#define TARGET_HAS_SIGN_EXTEND          1
#define TARGET_HAS_BLOCKMOVE            1   /* J_MOVC/J_CLRC: rep movs  */
#define TARGET_HAS_SWITCH_BRANCHTABLE   1
#define TARGET_HAS_NEGATIVE_INDEXING    1
#define TARGET_FP_LITS_FROM_MEMORY      1
#define TARGET_STACKS_LINK              1
#define TARGET_FLAGS_CALL_RESULTS       1   /* see gen.c J_CALLK       */
#define TARGET_CALLER_EXTENDS_NARROW_RESULTS 1
#define TARGET_LACKS_DIVIDE_LITERALS    1
#define TARGET_DOESNT_CHECK_SWIS        1
#define immed_cmp(n)                    1

/* Any 32-bit displacement is encodable.                                */
#define TARGET_LDRK_MIN                 (-0x7fffffffL-1)
#define TARGET_LDRK_MAX                 0x7fffffffL
#define TARGET_LDRWK_MIN                TARGET_LDRK_MIN
#define TARGET_LDRWK_MAX                TARGET_LDRK_MAX
#define TARGET_LDRFK_MIN                TARGET_LDRK_MIN
#define TARGET_LDRFK_MAX                TARGET_LDRK_MAX

/*
 * Registers.  mip needs each class (argument, temporary, variable) to be
 * a contiguous range of internal register numbers; the back end maps them
 * to the hardware registers.
 *
 *   internal:  0   1   2   3   4   5   6   7   8     9..16
 *   hardware: eax edx ecx ebx esi edi ebp esp flags xmm0..7
 *   use:      tmp tmp tmp var var var  fp  sp  psr   fp tmp
 *
 * Arguments are all passed on the stack (NARGREGS=0).  Integer results
 * come back in eax, and two-word results (e.g. long long) in edx:eax, so
 * edx is internal register 1.  FP results come back in st(0) as the ABI requires,
 * which the back end moves to/from xmm0 at calls and returns.
 */
#define R_A1            0L      /* eax */
#define NARGREGS        0L
#define NRESULTREGS     2L      /* edx:eax                              */
#define R_T1            0L      /* eax, edx, ecx: caller-saved temps    */
#define NTEMPREGS       3L
#define R_V1            3L      /* ebx, esi, edi: callee-saved          */
#define NVARREGS        3L
#define MAXGLOBINTREG   0L
#define R_IP            2L      /* ecx (mip needs some R_IP)            */
#define R_FP            6L      /* ebp */
#define R_SP            7L      /* esp */
#define R_LR            R_IP    /* not used: return address is stacked  */
#define R_PSR           8L      /* notional: the flags register         */
#define NINTREGS        9L

#define R_F0            9L      /* xmm0 */
#define NFLTARGREGS     0L
#define R_FT1           9L      /* xmm0-xmm7: all caller-saved          */
#define NFLTTEMPREGS    8L
#define R_FV1           17L
#define NFLTVARREGS     0L
#define MAXGLOBFLTREG   0L
#define NFLTREGS        8L

#define ALLOCATION_ORDER { 0, 1, 2, 3, 4, 5, \
                           9, 10, 11, 12, 13, 14, 15, 16, \
                           255 }

/* System V i386: structs are only as aligned as their members.       */
#define alignof_struct  1
#define alignof_double  4

#define target_lib_name_(x,e) (e)
extern char *target_asm_options(ToolEnv *);
#define target_asm_options_(x) target_asm_options(x)

#define TARGET_SUPPORTS_TOOLENVS

#ifdef CPLUSPLUS
#  define TOOLFILENAME "n++"
#else
#  define TOOLFILENAME "ncc"
#endif

#endif

/* end of x86/target.h */
