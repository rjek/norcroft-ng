/*
 * C compiler file meow/mcdep.c
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Machine-dependent queries and option handling for the MEOW backend.
 */

#include <ctype.h>
#include <string.h>

#include "globals.h"
#include "mcdep.h"
#include "mcdpriv.h"
#include "jopcode.h"
#include "toolenv.h"
#include "tooledit.h"
#include "errors.h"
#include "sem.h"
#include "regsets.h"

int32 pcs_flags;
int32 config;
int arthur_module;

typedef struct { char const *name; char const *val; } EnvItem;

static EnvItem const builtin_defaults[] = {
  {"-zi", "=2"},
  {NULL, NULL}
};

int mcdep_toolenv_insertdefaults(ToolEnv *t) {
  EnvItem const *p = builtin_defaults;
  for (; p->name != NULL; p++) {
    ToolEdit_InsertStatus rc = tooledit_insert(t, p->name, p->val);
    if (rc == TE_Failed) return 1;
  }
  return 0;
}

bool mcdep_config_option(char name, char const tail[], ToolEnv *t)
{   IGNORE(name); IGNORE(tail); IGNORE(t);
    return NO;
}

KW_Status mcdep_keyword(char const *key, char const *nextarg, ToolEnv *t) {
    IGNORE(key); IGNORE(nextarg); IGNORE(t);
    return KW_NONE;
}

void target_lib_variant(char *b) {
    strcpy(b, "");
}

char const *target_lib_name(ToolEnv *t, char const *name) {
    IGNORE(t);
    return name;
}

char *target_asm_options(ToolEnv *t) {
  IGNORE(t);
  return "";
}

void config_init(ToolEnv *t)
{
    IGNORE(t);
    config = CONFIG_SOFTWARE_FP;
    pcs_flags = PCS_SOFTFP | PCS_NOFP | PCS_NOSTACKCHECK;
}

void mcdep_set_options(ToolEnv *t) {
    IGNORE(t);
}

/* ---- queries used by the middle end ------------------------------------ */

bool sets_psr(const Icode *ic)
{   J_OPCODE op = ic->op & J_TABLE_BITS;
    return op == J_CMPR || op == J_CMPK || op == J_CASEBRANCH;
}

bool reads_psr(const Icode *ic)
{   J_OPCODE op = ic->op & J_TABLE_BITS;
    return op == J_B && (ic->op & Q_MASK) != 0;
}

bool uses_psr(const Icode *ic)
{   return reads_psr(ic) || sets_psr(ic);
}

bool corrupts_psr(const Icode *ic)
{   J_OPCODE op = ic->op & J_TABLE_BITS;
    return op == J_CALLK || op == J_CALLR || op == J_OPSYSK;
}

bool has_side_effects(Icode const *ic)
{
    J_OPCODE op = ic->op & J_TABLE_BITS;
    return writes_mem(op) != 0;
}

bool corrupts_r1(Icode const *ic)
{   J_OPCODE op = ic->op & J_TABLE_BITS;
    return op == J_MOVC || op == J_CLRC;
}

bool corrupts_r2(Icode const *ic)
{   J_OPCODE op = ic->op & J_TABLE_BITS;
    return op == J_MOVC;
}

bool UnalignedLoadMayUse(RealRegister r)
{
    return r == R_IP;
}

void remove_writeback(Icode *ic)
{
    IGNORE(ic);
}

/* No offset field on loads and stores, except that the backend will add
 * a stack offset itself. */
int32 MaxMemOffset(J_OPCODE op)
{
    return uses_stack(op) ? 4095 : 0;
}

int32 MinMemOffset(J_OPCODE op)
{
    IGNORE(op);
    return 0;
}

int32 MemQuantum(J_OPCODE op)
{
    if ((op & J_ALIGNMENT) == J_ALIGN1)
        return 1;
    switch(j_memsize(op))
    {
        case MEM_B: return 1;
        case MEM_W: return 2;
        default:    return 4;
    }
}

char *CheckJopcode(const Icode *ic, CheckJopcode_flags flags)
{
    char *errmsg = NULL;
    if ((flags & JCHK_REGS) != 0) {
        if ((reads_r1(ic->op) || loads_r1(ic->op)) && (uint32)ic->r1.rr >= 16)
            errmsg = "illegal register number";
        if ((reads_r2(ic->op) || loads_r2(ic->op)) && (uint32)ic->r2.rr >= 16)
            errmsg = "illegal register number";
        if (reads_r3(ic->op) && (uint32)ic->r3.rr >= 16)
            errmsg = "illegal register number";
    }
    if ((flags & JCHK_SYSERR) != 0 && errmsg != NULL)
        syserr(errmsg);
    return errmsg;
}

int multiply_cycles(int val, bool accumulate)
{
    IGNORE(val); IGNORE(accumulate);
    return 40;
}

int32 CheckSWIValue(int32 n)
{
    return n;
}

static uint32 resultregs(Icode const *ic)
{   /* result registers including the default result register */
    if (k_resultregs_(ic->r2.i) == 0) return regbit(ic->r1.r);
    return reglist(R_A1, k_resultregs_(ic->r2.i));
}

static uint32 argumentregs(uint32 argdesc)
{
    return reglist(R_A1, k_argregs_(argdesc));
}

/* Fixed register use.  Only calls matter: the backend's own scratch
 * registers (at, ir) are never allocated, and lr is reserved too. */
void RealRegisterUse(Icode const *ic, RealRegUse *u)
{
    uint32 use = 0, def = 0, c_in = 0, c_out = 0;
    uint32 volatile_regs = regbit(R_IP) | reglist(R_A1, NARGREGS) |
                           reglist(R_F0, NFLTARGREGS);

    switch (ic->op & J_TABLE_BITS) {
        case J_OPSYSK:
        case J_CALLK:
        case J_CALLR:
            use = argumentregs(ic->r2.i);
            def = resultregs(ic);
            c_in = regbit(R_LR);
            c_out = volatile_regs & ~resultregs(ic);
            break;
        case J_ENTER:
            def = argumentregs(ic->r3.i);
            break;
        default:
            break;
    }
    memclr(&u->use, sizeof(RealRegSet));
    memclr(&u->def, sizeof(RealRegSet));
    memclr(&u->c_in, sizeof(RealRegSet));
    memclr(&u->c_out, sizeof(RealRegSet));
    u->use.map[0] = use;
    u->def.map[0] = def;
    u->c_in.map[0] = c_in;
    u->c_out.map[0] = c_out;
}

Expr *rd_asm_decl(void)
{
    cc_err(gen_err_inlineasm);
    return errornode;
}

bool immed_cmp(int32 n)
{
    return n >= -128 && n <= 127;
}

/* end of meow/mcdep.c */
