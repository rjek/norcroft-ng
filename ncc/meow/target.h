/*
 * C compiler file meow/target.h
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * MEOW: a 32-bit RISC microcontroller with 16-bit instructions, sixteen
 * registers and no multiply, divide or floating point.  See the MEOW
 * repository's docs/reference.md and docs/abi.md.
 */

#ifndef _target_LOADED
#define _target_LOADED 1

#define TARGET_IS_MEOW 1
/* The middle end only tracks fixed register use (call arguments and
 * results) for targets that call themselves this. */
#define TARGET_IS_ARM_OR_THUMB 1

#include "toolenv.h"

#ifndef TARGET_MACHINE
#  define TARGET_MACHINE "MEOW"
#endif

#define TARGET_PREDEFINES { "__meow", \
                            "__CLK_TCK=100", \
                            "__JMP_BUF_SIZE=12" }

#define EXTENSION_SYSV 1

#define localcg_newliteralpool_exists 1

#define SOFTWARE_FLOATING_POINT 1
#define software_floating_point_enabled 1
#define software_floats_enabled 1
#define software_doubles_enabled 1
#define SOFTWARE_FLOATING_POINT_RETURNS_DOUBLES_IN_REGISTERS 1
#define TARGET_SOFTFP_SUPPORT_INCLUDES_REVERSE_OPS 1
#define TARGET_FP_ARGS_IN_FP_REGS 1
#define TARGET_HAS_IEEE         1

#define TARGET_IS_LITTLE_ENDIAN 1

#define TARGET_HAS_NATURALLY_ALIGNED_STATICS 1
#define TARGET_HAS_HALFWORD_INSTRUCTIONS 1
#define TARGET_HAS_BSS  1
#define CONST_DATA_IN_CODE 1
/* Without this the CSE pass does not know a MOVC reads its source. */
#define TARGET_HAS_BLOCKMOVE 1

/* Every ALU instruction reads and writes its first operand, so tell the
 * register allocator to try to make r1 and r2 the same register. */
#define TARGET_HAS_2ADDRESS_CODE        1
#define two_address_code(op) (((op) & J_TABLE_BITS) == J_SHLR || \
                              ((op) & J_TABLE_BITS) == J_SHRR || \
                              ((op) & J_TABLE_BITS) == J_RORR || \
                              ((op) & J_TABLE_BITS) == J_ADDR || \
                              ((op) & J_TABLE_BITS) == J_SUBR || \
                              ((op) & J_TABLE_BITS) == J_ANDR || \
                              ((op) & J_TABLE_BITS) == J_ORRR || \
                              ((op) & J_TABLE_BITS) == J_EORR)

#define TARGET_HAS_ROTATE               1
#define TARGET_STACK_MOVES_ONCE         1
#define DO_NOT_EXPLOIT_REGISTERS_PRESERVED_BY_CALLEE 1
#define TARGET_LACKS_RR_UNALIGNED_ACCESSES 1
#define TARGET_LACKS_UNSIGNED_FIX       1
#define TARGET_ADDRESSES_UNSIGNED       1

/* Loads and stores have no offset field: the middle end computes every
 * address into a register and the backend adds the odd stack offset. */
#define TARGET_LDRK_MIN                 0
#define TARGET_LDRK_MAX                 0
#define TARGET_LDRWK_MIN                0
#define TARGET_LDRWK_MAX                0
#define TARGET_LDRBK_MIN                0
#define TARGET_LDRBK_MAX                0
#define TARGET_SP_LDRK_MIN              0
#define TARGET_SP_LDRK_MAX              4095L
#define TARGET_SP_LDRWK_MIN             0
#define TARGET_SP_LDRWK_MAX             4095L
#define TARGET_SP_LDRBK_MIN             0
#define TARGET_SP_LDRBK_MAX             4095L

#define TARGET_MAX_FRAMESIZE            (7*4)

/* MABI register assignment. */
#define R_A1            0L
#define NARGREGS        4L
#define R_V1            4L
#define NVARREGS        6L
#define MAXGLOBINTREG   10L
#define R_IP            10L     /* at: assembler and compiler temporary */
#define R_SP            11L
#define R_LR            12L
#define R_IR            13L     /* immediate register, backend scratch */
#define R_PSR           14L
#define R_PC            15L
#define NINTREGS        16L

/* The middle end names these ARM registers; MABI has no frame pointer,
 * stack limit or static base, so they are never referenced at run time. */
#define R_FP            R_SP
#define R_SL            R_SP
#define R_SB            R_SP

/* Calling-standard flags, kept for the middle end's benefit. */
extern int32 pcs_flags;
#define PCS_CALLCHANGESPSR  1
#define PCS_FPE3            2
#define PCS_NOSTACKCHECK    4
#define PCS_REENTRANT       8
#define PCS_FPREGARGS       16
#define PCS_NOFP            32
#define PCS_SOFTFP          64
#define PCS_INTERWORK       128
#define PCS_ACCESS_CONSTDATA_WITH_ADR 256
#define PCS_ZH_MASK         0xff

/* Floating point is done in software, but the middle end still wants a
 * register file to talk about. */
#define R_F0            16L
#define NFLTARGREGS     8L
#define NFLTVARREGS     8L
#define R_FV1           (R_F0+NFLTARGREGS)
#define MAXGLOBFLTREG   0L

#define R_P1            R_A1

#define ALLOCATION_ORDER    {0,1,2,3,4,5,6,7,8,9,255}

#ifndef alignof_double
#  define alignof_double    4
#endif

#ifndef COMPILING_ON_ARM
   extern char const *target_lib_name(ToolEnv *, char const *);
#  define target_lib_name_(x,e) target_lib_name(x,e)
#endif
#define target_asm_options_(x) ""

#define TARGET_HAS_ELF 1

#define LDM_REGCOUNT_MAX_DEFAULT 16
#define LDM_REGCOUNT_MIN_DEFAULT  3

#define TARGET_SUPPORTS_TOOLENVS

#define TOOLNAME armcc  /* the driver has one entrypoint name, armccinit */
#define TOOLFILENAME "meowcc"

#endif

/* end of meow/target.h */
