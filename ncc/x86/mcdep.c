/*
 * x86/mcdep.c -- machine-dependent parts of the x86 back end that are
 * not the code generator proper: register constraints for the register
 * allocator, flags-register predicates, and configuration.
 * SPDX-Licence-Identifier: Apache-2.0
 */

#include <string.h>

#include "globals.h"
#include "mcdep.h"
#include "mcdpriv.h"
#include "jopcode.h"
#include "regalloc.h"
#include "flowgraf.h"
#include "toolenv.h"
#include "toolenv2.h"
#include "tooledit.h"

int32 config;
int32 pcs_flags;

/* ---------------------------------------------------------------- */
/* Register constraints                                               */
/* ---------------------------------------------------------------- */

/* Internal register numbers (see target.h).                          */
#ifdef TARGET_IS_X86_64
#define I_EAX 6
#define I_EDX 2
#define I_ECX 3
#define I_R10 7
#define I_R11 8                 /* scratch for unsigned conversions     */
#define I_XMM15 31
/* A call preserves only rbx, r12-r15 (and rbp, rsp).                  */
#define CALL_CORRUPTS (((1uL << NARGREGS) - 1) << R_A1 | \
                       ((1uL << NTEMPREGS) - 1) << R_T1)
#else
#define I_EAX 0
#define I_EDX 1
#define I_ECX 2
/* A call preserves only ebx, esi, edi (and ebp, esp).                 */
#define CALL_CORRUPTS (regbit(I_EAX) | regbit(I_ECX) | regbit(I_EDX))
#endif

/*
 * Some x86 instructions use fixed registers.  gen.c moves the operands
 * into place itself; here we stop the register allocator putting the
 * operands, the result, or anything live across the instruction, in the
 * registers it overwrites.
 */
void RealRegisterUse(Icode const *ic, RealRegUse *u)
{   uint32 c_in = 0, c_out = 0, def = 0, use = 0;
    switch (ic->op & J_TABLE_BITS)
    {
    case J_DIVR: case J_REMR:
        c_in = c_out = regbit(I_EAX) | regbit(I_EDX);
        break;
    case J_SHLR: case J_SHRR:
        c_in = c_out = regbit(I_ECX);
        break;
#ifdef TARGET_IS_X86_64
    case J_CASEBRANCH:              /* see gen.c                        */
        c_in = c_out = regbit(I_R10) | regbit(I_R11);
        break;
    case J_FLTFR: case J_FLTDR:
        if (ic->op & J_UNSIGNED) c_in = c_out = regbit(I_R11);
        break;
    case J_FIXFR: case J_FIXDR:
        if ((ic->op & (J_UNSIGNED|J_W32)) == J_UNSIGNED)
            c_in = c_out = regbit(I_XMM15);
        break;
#endif
    case J_CALLK: case J_CALLR:
        if (k_resultregs_(ic->r2.i) > 1)        /* e.g. edx:eax        */
            def = ((1uL << k_resultregs_(ic->r2.i)) - 1) << R_A1result;
        else if (isany_realreg_(ic->r1.r))
            def = regbit(ic->r1.rr);
        c_out = (CALL_CORRUPTS | (((1uL << NFLTREGS) - 1) << R_F0)) & ~def;
        /* The call reads its register arguments.                      */
        use = ((1uL << (k_argregs_(ic->r2.i) - k_fltregs_(ic->r2.i))) - 1) << R_A1 |
              ((1uL << k_fltregs_(ic->r2.i)) - 1) << R_F0;
        break;
    }
    memclr(u, sizeof(*u));
    u->def.map[0] = def;
    u->use.map[0] = use;
    u->c_in.map[0] = c_in;
    u->c_out.map[0] = c_out;
}

/* ---------------------------------------------------------------- */
/* The flags register                                                 */
/* ---------------------------------------------------------------- */

bool sets_psr(Icode const *ic)
{   return is_compare(ic->op & J_TABLE_BITS);
}

bool reads_psr(Icode const *ic)
{   return (ic->op & J_TABLE_BITS) == J_B && (ic->op & Q_MASK) != Q_AL;
}

bool uses_psr(Icode const *ic)
{   return sets_psr(ic) || reads_psr(ic);
}

/* Almost all x86 arithmetic changes the flags.                       */
bool corrupts_psr(Icode const *ic)
{   switch (ic->op & J_TABLE_BITS)
    {
    case J_MOVR: case J_MOVK: case J_ADCON: case J_STRING:
    case J_MOVFR: case J_MOVDR:
    case J_LABEL: case J_B: case J_NOOP:
        return NO;
    }
    if (j_is_ldr_or_str(ic->op & J_TABLE_BITS)) return NO;
    return !sets_psr(ic);
}

bool has_side_effects(Icode const *ic)
{   return writes_mem(ic->op & J_TABLE_BITS) != 0;
}

void remove_writeback(Icode *ic)
{   IGNORE(ic);
}

/* x86 can address with any 32-bit displacement.                     */
int32 MinMemOffset(J_OPCODE op)
{   IGNORE(op);
    return -0x7fffffffL;
}

int32 MaxMemOffset(J_OPCODE op)
{   IGNORE(op);
    return 0x7fffffffL;
}

int32 MemQuantum(J_OPCODE op)
{   IGNORE(op);
    return 1;
}

/* ---------------------------------------------------------------- */
/* Configuration                                                      */
/* ---------------------------------------------------------------- */

int mcdep_toolenv_insertdefaults(ToolEnv *t)
{   /* As the System V ABI requires: plain char is signed, and enums   */
    /* are the size of int.                                            */
    if (tooledit_insert(t, ".schar", "=-zc") == TE_Failed) return 1;
    if (tooledit_insert(t, ".enums", "=-fy") == TE_Failed) return 1;
    return 0;
}

KW_Status mcdep_keyword(char const *key, char const *nextarg, ToolEnv *t)
{   IGNORE(key); IGNORE(nextarg); IGNORE(t);
    return KW_NONE;
}

bool mcdep_config_option(char name, char const tail[], ToolEnv *t)
{   IGNORE(name); IGNORE(tail); IGNORE(t);
    return NO;
}

void config_init(ToolEnv *t)
{   IGNORE(t);
#ifdef TARGET_IS_X86_64
    /* Floating arguments are passed in xmm registers, and prototyped    */
    /* float arguments as floats.                                        */
    config = CONFIG_FPREGARGS | CONFIG_UNWIDENED_NARROW_ARGS;
#else
    config = 0;
#endif
    pcs_flags = 0;
}

void mcdep_set_options(ToolEnv *t)
{   IGNORE(t);
}

char *target_asm_options(ToolEnv *t)
{   IGNORE(t);
    return "";
}

/* end of x86/mcdep.c */
