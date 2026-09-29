/*
 * x86/target.h -- target description for the x86 back end.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Two targets: 32-bit i386 (System V i386 psABI), and x86-64 (System V
 * AMD64 psABI) if TARGET_IS_X86_64 is defined.  Both produce ELF via the
 * system assembler.  Floating point is done in SSE2 registers.
 */

#ifndef _target_LOADED
#define _target_LOADED 1

#include "toolenv.h"

#define TARGET_IS_X86                   1

#ifdef TARGET_IS_X86_64
#  define TARGET_MACHINE                "x86-64"
#  define TARGET_PREDEFINES             { "__x86_64", "__x86_64__", \
                                          "__amd64", "__amd64__", \
                                          "__LP64__", "_LP64", \
                                          "__ELF__", "__CHAR_BIT__=8", \
                                          "__SIZEOF_POINTER__=8", \
                                          "__SIZEOF_LONG__=8", \
                                          "__SIZEOF_LONG_LONG__=8", \
                                          "__ORDER_LITTLE_ENDIAN__=1234", \
                                          "__BYTE_ORDER__=1234" }
#else
#  define TARGET_IS_I386                1
#  define TARGET_MACHINE                "i386"
#  define TARGET_PREDEFINES             { "__i386", "__i386__", "i386", \
                                          "__ELF__", "__CHAR_BIT__=8", \
                                          "__SIZEOF_POINTER__=4", \
                                          "__SIZEOF_LONG__=4", \
                                          "__SIZEOF_LONG_LONG__=8", \
                                          "__ORDER_LITTLE_ENDIAN__=1234", \
                                          "__BYTE_ORDER__=1234" }
#endif

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
#define TARGET_ADDRESSES_UNSIGNED       1
#define TARGET_STACKS_LINK              1
#define TARGET_CALLER_EXTENDS_NARROW_RESULTS 1
#define TARGET_LACKS_DIVIDE_LITERALS    1
/* Almost all x86 instructions change the flags, so a switch's compares  */
/* mustn't leave them for use in later blocks (see casebranch()).        */
#define TARGET_LACKS_3WAY_COMPARE       1
#define TARGET_DOESNT_CHECK_SWIS        1
#define immed_cmp(n)                    1

/* Any 32-bit displacement is encodable.                                */
#define TARGET_LDRK_MIN                 (-0x7fffffffL-1)
#define TARGET_LDRK_MAX                 0x7fffffffL
#define TARGET_LDRWK_MIN                TARGET_LDRK_MIN
#define TARGET_LDRWK_MAX                TARGET_LDRK_MAX
#define TARGET_LDRFK_MIN                TARGET_LDRK_MIN
#define TARGET_LDRFK_MAX                TARGET_LDRK_MAX

#ifdef TARGET_IS_X86_64

/* 64-bit integer registers: long, long long and pointers are 64 bits.  */
/* Integer operations are 64-bit unless marked J_W32 (see jopcode.h).   */
#define TARGET_HAS_64BIT_INTREGS        1
#define sizeof_long                     8
#define sizeof_ptr                      8
#define alignof_longlong                8
#define alignof_double                  8
#define alignof_toplevel_auto           8   /* argument slots           */
#define alignof_max                     8

/* The first 8 floating arguments are in xmm0-7, and the first 6       */
/* integer ones in registers, counted separately; this is so for        */
/* variadic functions too (with al giving the number of xmm registers). */
#define TARGET_FP_ARGS_IN_FP_REGS       1
#define TARGET_VARIADIC_FP_ARGS_IN_FP_REGS 1

/* A variadic function saves all the argument registers, and             */
/* __builtin_va_start makes a va_list that describes them.               */
#define TARGET_HAS_SYSV_VA_START        1

/* Structs of up to 16 bytes are passed and returned in registers, as    */
/* the psABI says, and others in memory.  mip returns them in integer    */
/* registers rax and r10 (internal 6 and 7), and gen.c moves them to and */
/* from rax, rdx, xmm0 and xmm1 as the K_RESULTSSE flags say.            */
#define TARGET_HAS_SYSV_AMD64_ABI       1
#define MEMCPYREG                       INTREG
#define MEMCPYQUANTUM                   8

/*
 * Registers.  mip needs each class (argument, temporary, variable) to be
 * a contiguous range of internal register numbers; the back end maps them
 * to the hardware registers.
 *
 *   internal:  0   1   2   3   4   5   6   7   8   9   10  11  12  13
 *   hardware: rdi rsi rdx rcx r8  r9  rax r10 r11 rbx r12 r13 r14 r15
 *   use:      <------ arguments ----> <-temps-->  <--- callee-save --->
 *
 *   internal:  14   15         16..23     24..31
 *   hardware: rsp  rbp/flags  xmm0-7     xmm8-15
 *   use:      sp   fp/psr     fp args    fp temps
 *
 * rbp is never allocated, so it shares its number with the notional
 * flags register, which keeps everything within 32 registers.
 */
#define R_A1            0L      /* rdi */
#define NARGREGS        6L
#define R_A1result      6L      /* rax */
#define NRESULTREGS     2L      /* see below */
#define R_T1            6L      /* rax, r10, r11 */
#define NTEMPREGS       3L
#define R_V1            9L      /* rbx, r12-r15 */
#define NVARREGS        5L
#define MAXGLOBINTREG   0L
#define R_IP            8L      /* r11 (mip needs some R_IP)            */
#define R_SP            14L
#define R_FP            15L     /* rbp */
#define R_PSR           15L
#define R_LR            R_IP    /* not used: return address is stacked  */
#define NINTREGS        16L

#define R_F0            16L     /* xmm0 */
#define NFLTARGREGS     8L
#define R_FT1           24L     /* xmm8-15 */
#define NFLTTEMPREGS    8L
#define R_FV1           32L
#define NFLTVARREGS     0L
#define MAXGLOBFLTREG   0L
#define NFLTREGS        16L

#define ALLOCATION_ORDER { 6, 7, 8, 0, 1, 2, 3, 4, 5, 9, 10, 11, 12, 13, \
                           24, 25, 26, 27, 28, 29, 30, 31, \
                           16, 17, 18, 19, 20, 21, 22, 23, \
                           255 }

#else /* TARGET_IS_I386 */

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

#define alignof_double  4

/* i386 returns float and double results in st(0), and the callee pops  */
/* the pointer to a struct result.                                      */
#define TARGET_FLAGS_CALL_RESULTS       1   /* see gen.c J_CALLK       */

#endif /* TARGET_IS_X86_64 */

/* System V: structs are only as aligned as their members, and          */
/* bitfields are laid out as the ABIs say (see structfield()).          */
#define alignof_struct  1
#define TARGET_HAS_SYSV_BITFIELDS       1

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
