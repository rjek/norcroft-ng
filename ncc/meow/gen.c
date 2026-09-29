/*
 * C compiler file meow/gen.c
 * Copyright (C) Codemist Ltd., 1994.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Local code generator for MEOW.  Turns J opcodes into 16-bit MEOW
 * instructions in the code buffer.
 *
 * MEOW has no offset addressing, no flag-setting arithmetic, only one
 * conditional instruction (B, reach 512 bytes) and one way to load a
 * constant wider than 8 bits (LDI into ir, 12 bits).  So:
 *
 *  - r10 (at) is reserved as the backend's address temporary and r13 (ir)
 *    as its immediate temporary; neither is ever allocated;
 *  - forward branches are emitted short and, if they threaten to go out
 *    of reach, an island of LDI/ADD pc long jumps is planted and the
 *    branches are retargeted at it;
 *  - literals (addresses and large constants) live in pools reached by
 *    LDI/ADD ir, pc/LDR, so a pool must be within 2 KB of its users.
 *
 * exports:
 *   void show_instruction(Icode *ic);
 *   RealRegister local_base(Binder *b);
 *   int32 local_address(Binder *b);
 *   void setlabel(LabelNumber *);
 *   void branch_round_literals(LabelNumber *);
 */

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "globals.h"
#include "builtin.h"
#include "mcdep.h"
#include "target.h"
#include "mcdpriv.h"
#include "xrefs.h"
#include "jopcode.h"
#include "store.h"
#include "codebuf.h"
#include "regalloc.h"
#include "cg.h"
#include "simplify.h"
#include "errors.h"
#include "bind.h"
#include "aeops.h"
#include "meow_isa.h"

#define M_ARGREGS       (regbit(R_A1+NARGREGS)-regbit(R_A1))
#define M_VARREGS       (regbit(R_V1+NVARREGS)-regbit(R_V1))
#define NONLEAF (PROC_ARGPUSH | PROC_ARGADDR | PROC_BIGSTACK | BLKCALL)
#define ARGS2STACK (PROC_ARGPUSH | PROC_ARGADDR)
#define IS_VARIADIC(m) ((m) == 0xfff)

#define NoRegister ((RealRegister)-1)

/* Reach of a short branch and of an LDI-based long jump, with a margin
 * for the instruction that follows and for pool alignment. */
#define B_REACH   480L
#define LDI_REACH 2000L
#define POOL_REACH 1960L

/* Condition codes as MEOW encodes them. */
enum {
    C_EQ, C_NE, C_CS, C_CC, C_MI, C_PL, C_VS, C_VC,
    C_HI, C_LS, C_GE, C_LT, C_GT, C_LE, C_AL, C_NV
};

/* ---- per-function state ------------------------------------------------ */

static int32 fp_minus_sp;       /* frame bytes below the saved registers */
static int32 save_mask;         /* callee-saved registers pushed */
static bool lr_pushed;
static int32 pushed_args;       /* argument registers spilled at entry */
static LabelNumber *returnlab;
static bool return_pending;     /* a conditional return references returnlab */

int32 current_procnum;
List3 *label_values, *label_references;

static int32 mustlitby;         /* dump the literal pool before this codep */
static int32 mustbranchby;      /* plant an island before this codep */
static bool in_table;           /* inside a CASEBRANCH table: no dumps */

/* Forward references that may need an island. */
typedef struct FRef {
    struct FRef *cdr;
    int32 codep;                /* of the referencing instruction */
    int32 reach;
    LabelNumber *real_dest;
    LabelNumber *chained;       /* what the instruction currently targets */
} FRef;

static FRef *frefs;
static int32 nfrefs;

#define AddLabelReference(pos, lab) \
  label_references = (List3 *)syn_list3(label_references, (pos), \
        lab_name_(lab) & 0xfffff)

/* ---- code emission ----------------------------------------------------- */

static void outDCB(unsigned32 w)
{
    if ((codep & 3) == 0) {
        outcodeword(0, LIT_OPCODE);
        codep -= 4;
    }
    if ((codep & 1) == 0)
        code_flag_(codep) = LIT_BB;
    code_byte_(codep) = (unsigned char)w;
    codep += 1;
}

static void outHW(unsigned32 w)
{
    if (w & ~0xffffL) syserr("outHW(%lx)", (long)w);
    if (codep & 1) outDCB(0);
    if ((codep & 2) == 0) {
        outcodeword(0, LIT_OPCODE);
        codep -= 4;
    }
    code_flag_(codep) = LIT_OPCODE;
    code_hword_(codep) = (unsigned16)w;
    codep += 2;
}

#define NOP_WORD MEOW_ENCODE_MOV(0, 0, 0, 0, 0, 0)

void cnop(void)
{
    if (codep & 1) outDCB(0);
    if (((codebase+codep) & 2) != 0) outHW(NOP_WORD);
}

static void out_mov(RealRegister rd, RealRegister rs)
{
    if (rd != rs) outHW(MEOW_ENCODE_MOV(rd, 0, 0, 0, 0, rs));
}

static void out_ldi(int32 v)
{
    if (v < -2048 || v > 2047) syserr(syserr_displacement, (long)v);
    outHW(MEOW_ENCODE_LDI((uint32_t)v));
}

static void out_add3(bool sub, RealRegister rd, RealRegister rs, int32 imm)
{
    outHW(sub ? MEOW_ENCODE_SUB3(rd, imm, rs) : MEOW_ENCODE_ADD3(rd, imm, rs));
}

static void out_add8(bool sub, RealRegister rd, int32 imm)
{
    outHW(sub ? MEOW_ENCODE_SUB8(rd, imm) : MEOW_ENCODE_ADD8(rd, imm));
}

static void out_shift(RealRegister rd, bool arith, bool left, bool rot,
                      int32 amount)
{
    outHW(MEOW_ENCODE_SHI(arith, rd, left, rot, amount));
}

static void out_shiftr(RealRegister rd, bool arith, bool left, bool rot,
                       RealRegister rs)
{
    outHW(MEOW_ENCODE_SHR(arith, rd, left, rot, rs));
}

static void out_bitr(RealRegister rd, int op, bool inv, RealRegister rs)
{
    outHW(MEOW_ENCODE_BITR(inv, rd, op, rs));
}

static void out_biti(RealRegister rd, int op, bool inv, int32 bit)
{
    outHW(MEOW_ENCODE_BITI(inv, rd, op, bit));
}

/* size: 1, 2 or 4; high: halfword into the high half */
static void out_mem(bool store, RealRegister rv, RealRegister ra, int size,
                    bool high, int wb)
{
    unsigned half = size == 2;
    unsigned hilo = size == 1 ? 0 : size == 2 ? (high ? 0 : 1) : 1;
    unsigned w = wb == 2 || wb == 3;
    unsigned d = wb == 1 || wb == 3;

    outHW(MEOW_ENCODE_MEM(store, rv, half, hilo, w, d, ra));
}

static void out_push(RealRegister r)
{
    out_mem(YES, r, R_SP, 4, NO, 1);
}

static void out_pop(RealRegister r)
{
    out_mem(NO, r, R_SP, 4, NO, 3);
}

static void out_cmpk(RealRegister rn, int32 imm)
{
    outHW(MEOW_ENCODE_CMPI(rn, (uint32_t)imm));
}

static void out_cmpr(RealRegister rn, RealRegister rm)
{
    outHW(MEOW_ENCODE_CMPR(rn, 0, 0, rm));
}

static void out_tst(RealRegister rn, int bit)
{
    outHW(MEOW_ENCODE_TST(rn, 0, bit));
}

static void out_b(int cond, int32 halfwords)
{
    outHW(MEOW_ENCODE_B(cond, (uint32_t)halfwords));
}

static int bit_index(uint32_t v)
{
    int i;

    if (v == 0 || (v & (v - 1)) != 0) return -1;
    for (i = 0; (v & 1) == 0; i++) v >>= 1;
    return i;
}

static bool fits_simm(int32 v, int bits)
{
    int32 lim = (int32)1 << (bits - 1);

    return v >= -lim && v < lim;
}

/* ---- constants and arithmetic ------------------------------------------ */

static int load_integer0(RealRegister r, int32 n, bool emit);

/* r = n, using ir as scratch unless r is ir.  Same sequences as mas. */
static void load_integer(RealRegister r, int32 n)
{
    load_integer0(r, n, YES);
}

static int load_integer0(RealRegister r, int32 n, bool emit)
{
    uint32_t v = (uint32_t)n;
    int bit = bit_index(v);
    int chunks;
    int i;
    int len = 0;

#define EMIT(x) do { if (emit) { x; } len++; } while (0)
    if (n == 0) {
        EMIT(out_bitr(r, 3, NO, r));
        return len;
    }
    if (fits_simm(n, 12)) {
        EMIT(out_ldi(n));
        if (r != R_IR) EMIT(out_mov(r, R_IR));
        return len;
    }
    if (fits_simm(~n, 12)) {
        EMIT(out_ldi(~n));
        EMIT(out_bitr(r, 0, NO, R_IR));
        return len;
    }
    if (bit >= 0) {
        EMIT(out_bitr(r, 3, NO, r));
        EMIT(out_biti(r, 2, NO, bit));
        return len;
    }
    chunks = fits_simm(n >> 8, 12) ? 1 : fits_simm(n >> 16, 12) ? 2 : 3;
    EMIT(out_ldi(n >> (8 * chunks)));
    if (r != R_IR) EMIT(out_mov(r, R_IR));
    for (i = chunks; i > 0; i--) {
        int32 byte = (int32)((v >> (8 * (i - 1))) & 0xff);

        EMIT(out_shift(r, NO, YES, NO, 8));
        if (byte != 0) EMIT(out_add8(NO, r, byte));
    }
    return len;
#undef EMIT
}

/* rd = rs + n */
static void add_integer(RealRegister rd, RealRegister rs, int32 n)
{
    if (n == 0) {
        out_mov(rd, rs);
        return;
    }
    if (rd != rs && n >= 1 && n <= 15) {
        out_add3(NO, rd, rs, n);
        return;
    }
    if (rd != rs && n <= -1 && n >= -15) {
        out_add3(YES, rd, rs, -n);
        return;
    }
    out_mov(rd, rs);
    if (n >= 0 && n <= 255) {
        out_add8(NO, rd, n);
    } else if (n < 0 && n >= -255) {
        out_add8(YES, rd, -n);
    } else {
        load_integer(R_IR, n);
        out_add3(NO, rd, R_IR, 0);
    }
}

/* rd = rs OP k for AND/ORR/EOR (op 1, 2, 3). */
static void bit_integer(int op, RealRegister rd, RealRegister rs, int32 k)
{
    int bit = bit_index((uint32_t)k);
    uint32_t v = (uint32_t)k;

    out_mov(rd, rs);
    if (op == 1 && k == 0) {
        out_bitr(rd, 3, NO, rd);
        return;
    }
    if ((op == 2 || op == 3) && k == 0) return;
    if (op == 1 && k == -1) return;
    if (bit >= 0) {
        out_biti(rd, op, NO, bit);
        return;
    }
    if (op == 1 && bit_index(v + 1) > 0) {
        /* low-order mask: shift out the top */
        int n = bit_index(v + 1);

        out_shift(rd, NO, YES, NO, 32 - n);
        out_shift(rd, NO, NO, NO, 32 - n);
        return;
    }
    if (op == 1 && bit_index(~v + 1) > 0) {
        /* high-order mask: shift out the bottom */
        int n = bit_index(~v + 1);

        out_shift(rd, NO, NO, NO, n);
        out_shift(rd, NO, YES, NO, n);
        return;
    }
    if (op == 1 && bit_index(~v) >= 0) {
        out_biti(rd, 1, YES, bit_index(~v));
        return;
    }
    load_integer(R_IR, k);
    out_bitr(rd, op, NO, R_IR);
}

/* rd = rs * k by shifts and adds. */
/* rd = rs * k by Horner's scheme over the signed digits of k: shift the
 * running product up to the next nonzero digit, then add or subtract rs.
 * A run of ones costs one subtraction instead of one add per bit. */
static void multiply_integer(RealRegister rd, RealRegister rs, int32 k)
{
    uint32_t v = (uint32_t)(k < 0 ? -k : k);
    int digit[34];
    int n = 0, i, top, gap;
    RealRegister src = rs;

    if (k == 0) {
        load_integer(rd, 0);
        return;
    }
    while (v != 0) {
        if ((v & 1) != 0) {
            digit[n] = (v & 3) == 1 ? 1 : -1;
            v -= (uint32_t)digit[n];
        } else {
            digit[n] = 0;
        }
        v >>= 1;
        n++;
    }
    top = n - 1;                        /* always a +1 */
    if (rd == rs) {
        out_mov(R_IR, rs);              /* rd already holds rs; keep a copy */
        src = R_IR;
    } else {
        out_mov(rd, src);
    }
    gap = 0;
    for (i = top - 1; i >= 0; i--) {
        gap++;
        if (digit[i] == 0) continue;
        out_shift(rd, NO, YES, NO, gap);
        gap = 0;
        out_add3(digit[i] < 0, rd, src, 0);
    }
    if (gap > 0) out_shift(rd, NO, YES, NO, gap);
    if (k < 0) {
        out_bitr(rd, 0, NO, rd);
        out_add8(NO, rd, 1);
    }
}

/* ---- labels, branches and islands -------------------------------------- */

#define LABREF_B    0x01000000  /* 9-bit halfword offset in a B */
#define LABREF_LDI  0x02000000  /* LDI whose value is dest + imm - (q+2) */

static void setlabel2(LabelNumber *ll, int32 pos)
{
    label_values = (List3 *)binder_icons3(label_values, pos,
                                         lab_name_(ll) & 0xfffff);
    lab_setloc_(ll, pos | 0x80000000);
}

/* Patch every reference to l as pointing at codep. */
static void setlabel1(LabelNumber *l)
{
    List *p = l->u.frefs;

    while (p) {
        int32 v = car_(p);
        int32 q = v & 0x00ffffff;
        int32 w;
        int32 d;

        switch (v & 0xff000000) {
        case LABREF_B:
            w = code_hword_(q);
            d = (codep - q) >> 1;
            if (d < -256 || d > 255) syserr(syserr_displacement, (long)d);
            w = (w & ~0x1ff) | (d & 0x1ff);
            code_hword_(q) = (unsigned16)w;
            break;
        case LABREF_LDI:
            w = code_hword_(q);
            d = codep + MEOW_LDI_IMM_S(w) - (q + 2);
            if (d < -2048 || d > 2047) syserr(syserr_displacement, (long)d);
            code_hword_(q) = (unsigned16)MEOW_ENCODE_LDI((uint32_t)d);
            break;
        }
        p = (List *)discard2(p);
    }
    setlabel2(l, codep);
}

static void recalc_mustbranchby(void)
{
    FRef *f;
    int32 by = 0x10000000;
    int32 island = 4 * nfrefs + 8;

    for (f = frefs; f != NULL; f = cdr_(f))
        if (f->codep + f->reach - island < by)
            by = f->codep + f->reach - island;
    mustbranchby = by;
}

void setlabel(LabelNumber *l)
{
    FRef *f;
    FRef **fp = &frefs;

    while ((f = *fp) != NULL) {
        if (f->real_dest == l) {
            *fp = cdr_(f);
            nfrefs--;
            setlabel1(f->chained);
        } else
            fp = &cdr_(f);
    }
    recalc_mustbranchby();
    setlabel1(l);
}

/* A short branch to an unset label: record it so an island can rescue it. */
static void add_fref(LabelNumber *dest, int32 reach)
{
    FRef *f = NewSyn(FRef);

    cdr_(f) = frefs;
    f->codep = codep;
    f->reach = reach;
    f->real_dest = dest;
    f->chained = nextlabel();
    frefs = f;
    nfrefs++;
    AddLabelReference(codep, f->chained);
    addfref_(f->chained, codep | (reach == B_REACH ? LABREF_B : LABREF_LDI));
    recalc_mustbranchby();
}

static int32 label_pos(LabelNumber *l)
{
    return l->u.defn & 0x00ffffff;
}

/* LDI #d; ADD pc, ir  to a set label. */
static void long_jump(LabelNumber *dest)
{
    int32 d = label_pos(dest) - (codep + 2);

    if (d < -2048 || d > 2047) syserr(syserr_displacement, (long)d);
    out_ldi(d);
    out_add3(NO, R_PC, R_IR, 0);
}

static void branch_to(int cond, LabelNumber *dest)
{
    if (lab_isset_(dest)) {
        int32 d = label_pos(dest) - codep;

        if (d >= -512 && d <= 510) {
            AddLabelReference(codep, dest);
            out_b(cond, d / 2);
        } else if (cond == C_AL) {
            long_jump(dest);
        } else {
            LabelNumber *skip = nextlabel();

            AddLabelReference(codep, skip);
            out_b(cond ^ 1, 3);
            long_jump(dest);
            setlabel2(skip, codep);
        }
    } else {
        add_fref(dest, B_REACH);
        out_b(cond, 0);
    }
}

/* Plant an island: every pending short reference is pointed at a long
 * jump here, which in turn becomes the pending reference. */
static void dump_island(void)
{
    FRef *f;
    int32 size = 4 * nfrefs;
    LabelNumber *skip;

    if (nfrefs == 0) return;
    skip = nextlabel();
    if (size + 2 <= 510) {
        AddLabelReference(codep, skip);
        out_b(C_AL, (size + 2) / 2);
    } else {
        out_ldi(size + 2);
        out_add3(NO, R_PC, R_IR, 0);
    }
    for (f = frefs; f != NULL; f = cdr_(f)) {
        setlabel1(f->chained);
        f->chained = nextlabel();
        f->codep = codep;
        f->reach = LDI_REACH;
        addfref_(f->chained, codep | LABREF_LDI);
        out_ldi(0);
        out_add3(NO, R_PC, R_IR, 0);
    }
    setlabel2(skip, codep);
    recalc_mustbranchby();
}

/* ---- literal pool ------------------------------------------------------ */

static void dumplits(bool needs_jump)
{
    LabelNumber *skip = NULL;
    int32 bytes = litpoolp * 4 + 2;

    if (litpoolp == 0) {
        mustlitby = 0x10000000;
        return;
    }
    if (needs_jump) {
        skip = nextlabel();
        if (bytes + 2 <= 510) {
            AddLabelReference(codep, skip);
            out_b(C_AL, 0);
            addfref_(skip, (codep - 2) | LABREF_B);
        } else {
            addfref_(skip, codep | LABREF_LDI);
            out_ldi(0);
            out_add3(NO, R_PC, R_IR, 0);
        }
    }
    dumplits2(NO);
    if (needs_jump) setlabel(skip);
    mustlitby = 0x10000000;
}

/* "I can address up to n bytes beyond where I am now." */
static void addressability(int32 n)
{
    int32 litpoolsize = 4 * litpoolp;

    if (litpoolsize + 4 * nfrefs + 16 >= n) dumplits(YES);
    if (codep - litpoolsize + n < mustlitby) mustlitby = codep - litpoolsize + n;
}

/* ir = address of pool entry disp; then the caller loads through it. */
static void ldi_pool_ref(int32 disp)
{
    addfref_(litlab, codep | LABREF_LDI);
    out_ldi(disp);
}

/* rd = *(pool + disp) */
static void load_lit(RealRegister rd, int32 disp)
{
    ldi_pool_ref(disp);
    out_add3(NO, R_IR, R_PC, 0);
    out_mem(NO, rd, R_IR, 4, NO, 0);
}

/* rd = pool + disp, for strings and floating literals in the pool. */
static void pool_address(RealRegister rd, int32 disp)
{
    ldi_pool_ref(disp);
    out_mov(rd, R_PC);
    out_add3(NO, rd, R_IR, 0);
}

static void load_adcon(RealRegister rd, Symstr *name, int32 offset)
{
    int32 i;

    i = lit_findword(offset, LIT_ADCON, name,
                     LITF_INCODE|LITF_FIRST|LITF_LAST|LITF_PEEK);
    if (i < 0) {
        addressability(POOL_REACH);
        i = lit_findword(offset, LIT_ADCON, name,
                         LITF_INCODE|LITF_FIRST|LITF_LAST|LITF_NEW);
    }
    load_lit(rd, i);
}

/* A double is stored little-endian throughout: low word first, in memory,
 * in the pool and in a register pair. */
static void load_fp_adcon(RealRegister rd, J_OPCODE op, FloatCon *fc)
{
    int32 disp;
    int size = (op == J_MOVDK || op == J_ADCOND) ? 2 : 1;
    int32 words[2];

    words[0] = size == 2 ? fc->floatbin.db.lsd : fc->floatbin.irep[0];
    words[1] = fc->floatbin.db.msd;
    addressability(POOL_REACH);
    disp = lit_findwordsincurpool(words, size, LIT_FPNUM);
    if (disp < 0) {
        if (size == 1) {
            disp = lit_findwordaux(fc->floatbin.fb.val, LIT_FPNUM,
                                   fc->floatstr,
                                   LITF_INCODE|LITF_FIRST|LITF_LAST);
        } else {
            (void)lit_findwordaux(fc->floatbin.db.lsd, LIT_FPNUM1,
                                  fc->floatstr, LITF_INCODE|LITF_FIRST);
            disp = lit_findwordaux(fc->floatbin.db.msd, LIT_FPNUM2,
                                   fc->floatstr, LITF_INCODE|LITF_LAST) - 4;
        }
    }
    pool_address(rd, disp);
}

static void load_ll_adcon(RealRegister rd, Int64Con *c)
{
    int32 *w = (int32 *)&c->bin.i;
    int32 disp;

    addressability(POOL_REACH);
    disp = lit_findwordsincurpool(w, 2, LIT_INT64_1);
    if (disp < 0) {
        (void)lit_findword(w[0], LIT_INT64_1, NULL, LITF_INCODE|LITF_FIRST);
        disp = lit_findword(w[1], LIT_INT64_2, NULL, LITF_INCODE|LITF_LAST) - 4;
    }
    pool_address(rd, disp);
}

static void load_string(RealRegister rd, StringSegList *s, int32 extra)
{
    int32 disp = lit_findstringincurpool(s);

    if (disp < 0) {
        addressability(POOL_REACH);
        disp = litpoolp << 2;
        codeseg_stringsegs(s, 1);
    }
    pool_address(rd, disp + extra);
}

/* ---- copies that the next instruction can do without ---------------------- */

static bool skip_next, skip_after_next;

/* rd = rs & (1 << b) followed by an equality test of rd against zero
 * where rd dies: TST rs, #bit does the lot. */
static bool test_bit(RealRegister rd, RealRegister rs, int32 mask)
{
    Icode const *n = cg_next_icode;
    int bit = bit_index((uint32_t)mask);
    int32 cond;

    if (n == NULL || bit < 0) return NO;
    if ((n->op & J_TABLE_BITS) != J_CMPK || n->r3.i != 0) return NO;
    if (register_number(n->r2.r) != rd || (n->op & J_DEAD_R2) == 0) return NO;
    cond = n->op & Q_MASK & ~Q_UBIT;
    if (cond != Q_EQ && cond != Q_NE) return NO;
    out_tst(rs, bit);
    skip_next = YES;
    return YES;
}

/* MOV rd, rs followed by an instruction that reads rd for the last time
 * and does not write it: the reader uses rs instead and the copy goes. */
static RealRegister fwd_from = NoRegister, fwd_to;

/* MOV rd, rs; rs = rd +- k; CMP rd, #c with rd dying there, which is what
 * "n-- > 0" comes out as: compare rs before stepping it and lose the
 * copy. Nothing between sets or reads the flags. */
static bool step_after_test(RealRegister rd, RealRegister rs)
{
    Icode const *n = cg_next_icode, *n2;
    J_OPCODE nop;

    if (n == NULL || cg_next_count < 2 || rd == rs) return NO;
    n2 = n + 1;
    nop = n->op & J_TABLE_BITS;
    if (nop != J_ADDK && nop != J_SUBK) return NO;
    if (register_number(n->r1.r) != rs || register_number(n->r2.r) != rd)
        return NO;
    if ((n2->op & J_TABLE_BITS) != J_CMPK || (n2->op & J_DEAD_R2) == 0 ||
        register_number(n2->r2.r) != rd)
        return NO;
    if (n2->r3.i < -128 || n2->r3.i > 127) return NO;
    out_cmpk(rs, n2->r3.i);
    fwd_from = rd;
    fwd_to = rs;
    skip_after_next = YES;
    return YES;
}

static bool forward_copy(RealRegister rd, RealRegister rs)
{
    Icode const *n = cg_next_icode;
    J_OPCODE nop;
    bool used = NO;

    if (n == NULL || rd == rs) return NO;
    nop = n->op & J_TABLE_BITS;
    switch (nop) {
    case J_CALLK: case J_CALLR: case J_OPSYSK: case J_MOVC: case J_CLRC:
    case J_CASEBRANCH: case J_LABEL: case J_B: case J_ENDPROC: case J_ENTER:
    case J_PUSHM: case J_SETSP: case J_SETSPENV:
        return NO;
    default:
        break;
    }
    if (loads_r1(nop) && register_number(n->r1.r) == rd) return NO;
    if (loads_r2(nop) && register_number(n->r2.r) == rd) return NO;
    if (reads_r1(nop) && register_number(n->r1.r) == rd) {
        if ((n->op & J_DEAD_R1) == 0) return NO;
        used = YES;
    }
    if (reads_r2(nop) && register_number(n->r2.r) == rd) {
        if ((n->op & J_DEAD_R2) == 0) return NO;
        used = YES;
    }
    if (reads_r3(nop) && register_number(n->r3.r) == rd) {
        if ((n->op & J_DEAD_R3) == 0) return NO;
        used = YES;
    }
    if (!used) return NO;
    fwd_from = rd;
    fwd_to = rs;
    return YES;
}

/* ---- 64-bit helpers done in line ---------------------------------------- */

/* A MOVK into a3 just before a call is the shift count of a 64-bit shift;
 * remembering it lets the shift be done in line. */
static bool a3_known;
static int32 a3_value;

static void note_a3(J_OPCODE op1, RealRegister r1, RealRegister r2, int32 mi)
{
    if (op1 == J_MOVK && r1 == R_A1 + 2) {
        a3_known = YES;
        a3_value = mi;
    } else if (op1 == J_LABEL || op1 == J_CALLK || op1 == J_CALLR ||
               op1 == J_OPSYSK || op1 == J_MOVC || op1 == J_CLRC ||
               (loads_r1(op1) && r1 == R_A1 + 2) ||
               (loads_r2(op1) && r2 == R_A1 + 2)) {
        a3_known = NO;
    }
}

static bool is_ll(Symstr *name, Expr *fn)
{
    return name == bindsym_(exb_(fn));
}

/* a1:a2 op= a3:a4 for and, or, eor */
static void ll_bitop(int op)
{
    out_bitr(R_A1, op, NO, R_A1 + 2);
    out_bitr(R_A1 + 1, op, NO, R_A1 + 3);
}

/* a1:a2 <<= n, >>= n, with n a constant */
static void ll_shift(int32 n, bool left, bool arith)
{
    RealRegister lo = R_A1, hi = R_A1 + 1;

    n &= 63;
    if (n == 0) return;
    if (n >= 32) {
        if (left) {
            out_mov(hi, lo);
            if (n > 32) out_shift(hi, NO, YES, NO, n - 32);
            out_bitr(lo, 3, NO, lo);
        } else {
            out_mov(lo, hi);
            if (n > 32) out_shift(lo, arith, NO, NO, n - 32);
            if (arith) out_shift(hi, YES, NO, NO, 31);
            else out_bitr(hi, 3, NO, hi);
        }
        return;
    }
    if (left) {
        out_shift(hi, NO, YES, NO, n);
        out_mov(R_IR, lo);
        out_shift(R_IR, NO, NO, NO, 32 - n);
        out_bitr(hi, 2, NO, R_IR);
        out_shift(lo, NO, YES, NO, n);
    } else {
        out_shift(lo, NO, NO, NO, n);
        out_mov(R_IR, hi);
        out_shift(R_IR, NO, YES, NO, 32 - n);
        out_bitr(lo, 2, NO, R_IR);
        out_shift(hi, arith, NO, NO, n);
    }
}

/* The next instruction only tests a1 against zero: an equality compare
 * can then leave any nonzero value for "differs". */
static bool result_only_tested(void)
{
    Icode const *n = cg_next_icode;

    return n != NULL && (n->op & J_TABLE_BITS) == J_CMPK &&
           register_number(n->r2.r) == R_A1 && n->r3.i == 0 &&
           (n->op & J_DEAD_R2) != 0;
}

static bool inline_ll(Symstr *name)
{
    LabelNumber *l;

    if (is_ll(name, sim.lland)) { ll_bitop(1); return YES; }
    if (is_ll(name, sim.llor)) { ll_bitop(2); return YES; }
    if (is_ll(name, sim.lleor)) { ll_bitop(3); return YES; }
    if (is_ll(name, sim.llnot)) {
        out_bitr(R_A1, 0, NO, R_A1);
        out_bitr(R_A1 + 1, 0, NO, R_A1 + 1);
        return YES;
    }
    if (is_ll(name, sim.lltol)) return YES;
    if (is_ll(name, sim.llfromu)) {
        out_bitr(R_A1 + 1, 3, NO, R_A1 + 1);
        return YES;
    }
    if (is_ll(name, sim.llfroml)) {
        out_mov(R_A1 + 1, R_A1);
        out_shift(R_A1 + 1, YES, NO, NO, 31);
        return YES;
    }
    if (is_ll(name, sim.lladd)) {
        out_add3(NO, R_A1, R_A1 + 2, 0);
        out_cmpr(R_A1, R_A1 + 2);      /* carry: the sum came out below an operand */
        l = nextlabel();
        branch_to(C_CS, l);
        out_add8(NO, R_A1 + 1, 1);
        setlabel(l);
        out_add3(NO, R_A1 + 1, R_A1 + 3, 0);
        return YES;
    }
    if (is_ll(name, sim.llsub) || is_ll(name, sim.llrsb)) {
        RealRegister alo = R_A1, ahi = R_A1 + 1, blo = R_A1 + 2, bhi = R_A1 + 3;

        if (is_ll(name, sim.llrsb)) {   /* b - a: compute into b's registers */
            out_cmpr(blo, alo);
            out_add3(YES, blo, alo, 0);
            out_add3(YES, bhi, ahi, 0);
            l = nextlabel();
            branch_to(C_CS, l);
            out_add8(YES, bhi, 1);
            setlabel(l);
            out_mov(alo, blo);
            out_mov(ahi, bhi);
            return YES;
        }
        out_cmpr(alo, blo);
        out_add3(YES, alo, blo, 0);
        out_add3(YES, ahi, bhi, 0);
        l = nextlabel();
        branch_to(C_CS, l);
        out_add8(YES, ahi, 1);
        setlabel(l);
        return YES;
    }
    if (is_ll(name, sim.llneg)) {
        out_bitr(R_A1, 0, NO, R_A1);
        out_bitr(R_A1 + 1, 0, NO, R_A1 + 1);
        out_add8(NO, R_A1, 1);
        out_cmpk(R_A1, 0);
        l = nextlabel();
        branch_to(C_NE, l);
        out_add8(NO, R_A1 + 1, 1);
        setlabel(l);
        return YES;
    }
    if (a3_known && (is_ll(name, sim.llshiftl) || is_ll(name, sim.llushiftr) ||
                     is_ll(name, sim.llsshiftr))) {
        ll_shift(a3_value, is_ll(name, sim.llshiftl), is_ll(name, sim.llsshiftr));
        return YES;
    }
    if (is_ll(name, sim.llcmpne) && result_only_tested()) {
        /* nonzero iff they differ, which is all the test that follows wants */
        ll_bitop(3);
        out_bitr(R_A1, 2, NO, R_A1 + 1);
        return YES;
    }
    if (is_ll(name, sim.llcmpeq) || is_ll(name, sim.llcmpne)) {
        /* proper 0 or 1, as the library gives */
        ll_bitop(3);
        out_bitr(R_A1, 2, NO, R_A1 + 1);
        out_cmpk(R_A1, 0);
        load_integer(R_A1 + 1, is_ll(name, sim.llcmpeq) ? 1 : 0);
        l = nextlabel();
        branch_to(C_EQ, l);
        load_integer(R_A1 + 1, is_ll(name, sim.llcmpeq) ? 0 : 1);
        setlabel(l);
        out_mov(R_A1, R_A1 + 1);
        return YES;
    }
    return NO;
}

/* ---- calls ------------------------------------------------------------- */

static void call_k(Symstr *name)
{
    int32 d = obj_symref(name, xr_code, 0);

    if (d >= 0) {
        int32 delta = d - (codebase + codep + 4);
        int32 bdelta = d - (codebase + codep + 2);

        if (bdelta >= -512 && bdelta <= 510) {
            out_add3(NO, R_LR, R_PC, 4);    /* ADD lr, pc, #4; B target */
            out_b(C_AL, bdelta / 2);
            return;
        }
        if (delta >= -2048 && delta <= 2047) {
            out_ldi(delta);
            out_add3(NO, R_LR, R_PC, 4);
            out_add3(NO, R_PC, R_IR, 0);
            return;
        }
    }
    {
        int32 i = lit_findword(0, LIT_ADCON, name,
                               LITF_INCODE|LITF_FIRST|LITF_LAST|LITF_PEEK);

        if (i < 0) {
            addressability(POOL_REACH);
            i = lit_findword(0, LIT_ADCON, name,
                             LITF_INCODE|LITF_FIRST|LITF_LAST|LITF_NEW);
        }
        ldi_pool_ref(i);
        out_add3(NO, R_IR, R_PC, 0);
        out_add3(NO, R_LR, R_PC, 4);
        out_mem(NO, R_PC, R_IR, 4, NO, 0);
    }
}

static void call_r(RealRegister r)
{
    out_add3(NO, R_LR, R_PC, 4);
    out_mov(R_PC, r);
}

/* ---- frames ------------------------------------------------------------ */

RealRegister local_base(Binder const *b)
{
    IGNORE(b);
    return R_SP;
}

static int32 saved_bytes(void)
{
    return 4 * bitcount(save_mask) + (lr_pushed ? 4 : 0);
}

int32 local_address(Binder const *b)
{
    int32 p = bindaddr_(b);
    int32 n = p & ~BINDADDR_MASK;
    int32 base;

    switch (p & BINDADDR_MASK)
    {
default:
        syserr(syserr_local_addr, (long)p);
case BINDADDR_LOC:
        return fp_minus_sp - n;
case BINDADDR_ARG:
        base = fp_minus_sp + saved_bytes();
        if (n < 4 * NARGREGS) {
            if (n >= 4 * pushed_args) syserr(syserr_local_addr, (long)p);
            return base + n;
        }
        return base + 4 * pushed_args + n - 4 * NARGREGS;
    }
}

static void routine_entry(int32 m)
{
    int32 argwords = k_argwords_(m);
    int32 r;

    if (procauxflags & bitoffnaux_(s_irq))
        cc_err(gen_err_irq, compiler_name());
    current_procnum++;
    label_values = label_references = NULL;
    frefs = NULL;
    nfrefs = 0;
    return_pending = NO;
    save_mask = regmask & M_VARREGS;
    lr_pushed = (regmask & regbit(R_LR)) != 0 || (procflags & BLKCALL) != 0;
    pushed_args = 0;
    if (procflags & ARGS2STACK) {
        pushed_args = IS_VARIADIC(argwords) ? NARGREGS :
                      argwords > NARGREGS ? NARGREGS : argwords;
        for (r = pushed_args - 1; r >= 0; r--) out_push(R_A1 + r);
    }
    if (lr_pushed) out_push(R_LR);
    for (r = R_V1 + NVARREGS - 1; r >= R_V1; r--)
        if (save_mask & regbit(r)) out_push(r);
    fp_minus_sp = (greatest_stackdepth + 3) & ~3;
    add_integer(R_SP, R_SP, -fp_minus_sp);
}

static void routine_exit(void)
{
    int32 r;

    add_integer(R_SP, R_SP, fp_minus_sp);
    for (r = R_V1; r < R_V1 + NVARREGS; r++)
        if (save_mask & regbit(r)) out_pop(r);
    if (lr_pushed && pushed_args == 0) {
        out_pop(R_PC);                  /* the return address goes straight to pc */
        return;
    }
    if (lr_pushed) out_pop(R_LR);
    if (pushed_args != 0) add_integer(R_SP, R_SP, 4 * pushed_args);
    out_mov(R_PC, R_LR);
}

/* ---- memory access ----------------------------------------------------- */

/* The address is formed in ir: no other register is free, at being
 * allocatable.  A big offset goes in by LDI first, then the base. */
static RealRegister address_of(RealRegister rb, int32 off)
{
    if (off == 0) return rb;
    if (off >= -255 && off <= 255) {
        add_integer(R_IR, rb, off);
    } else {
        load_integer(R_IR, off);
        out_add3(NO, R_IR, rb, 0);
    }
    return R_IR;
}

static void sign_extend(RealRegister r, int bits)
{
    out_shift(r, NO, YES, NO, 32 - bits);
    out_shift(r, YES, NO, NO, 32 - bits);
}

static int mem_size(J_OPCODE op)
{
    switch (j_memsize(op & J_TABLE_BITS)) {
    case MEM_B: return 1;
    case MEM_W: return 2;
    default:    return 4;
    }
}

static bool mem_packed(J_OPCODE op)
{
    return mem_size(op) > 1 && (op & J_ALIGNMENT) == J_ALIGN1 &&
           (op & J_BASEALIGN4) == 0;
}

/* wb: 0 plain, 1 [rb, #-size]!, 2 [rb], #-size, 3 [rb], #size */
static void mem_op(J_OPCODE op, RealRegister rv, RealRegister rb, int32 off,
                   int wb)
{
    bool store = writes_mem(op & J_TABLE_BITS) != 0;
    int size = mem_size(op);
    RealRegister ra;

    ra = address_of(rb, off);
    if (mem_packed(op)) {
        /* packed data: a byte at a time, walking up with post-increment */
        int i;

        if (ra != R_IP) out_mov(R_IP, ra);
        if (store) {
            out_mem(YES, rv, R_IP, 1, NO, 3);
            for (i = 1; i < size; i++) {
                out_mov(R_IR, rv);
                out_shift(R_IR, NO, NO, NO, 8 * i);
                out_mem(YES, R_IR, R_IP, 1, NO, 3);
            }
        } else {
            out_mem(NO, R_IR, R_IP, 1, NO, 3);
            for (i = 1; i < size; i++) {
                out_mem(NO, rv, R_IP, 1, NO, 3);
                out_shift(rv, NO, YES, NO, 8 * i);
                out_bitr(R_IR, 2, NO, rv);
            }
            out_mov(rv, R_IR);
            if ((op & J_SIGNED) && size == 2) sign_extend(rv, 16);
        }
        return;
    }
    if (!store && (op & J_SIGNED) && size == 2) {
        /* into the top half, then one arithmetic shift sign extends */
        out_mem(NO, rv, ra, 2, YES, wb);
        out_shift(rv, YES, NO, NO, 16);
        return;
    }
    out_mem(store, rv, ra, size, NO, wb);
    if (!store && (op & J_SIGNED) && size == 1) sign_extend(rv, 8);
}

static bool is_ldrk_strk(J_OPCODE op)
{
    switch (op & J_TABLE_BITS) {
    case J_LDRK: case J_LDRBK: case J_LDRWK:
    case J_STRK: case J_STRBK: case J_STRWK:
        return YES;
    default:
        return NO;
    }
}

/* A plain access through rb followed by rb += size (or -= size) is one
 * post-indexed access.  The second instruction is then skipped. */

static bool fuse_post(J_OPCODE op, RealRegister rv, RealRegister rb, int32 off)
{
    Icode const *n = cg_next_icode;
    int size = mem_size(op);
    bool store = writes_mem(op & J_TABLE_BITS) != 0;

    if (n == NULL || off != 0 || mem_packed(op)) return NO;
    /* the block's own copy still names virtual registers */
    if ((n->op & J_TABLE_BITS) != J_ADDK || register_number(n->r1.r) != rb ||
        register_number(n->r2.r) != rb)
        return NO;
    if (n->r3.i != size && n->r3.i != -size) return NO;
    if (!store && rv == rb) return NO;
    mem_op(op, rv, rb, 0, n->r3.i == size ? 3 : 2);
    skip_next = YES;
    return YES;
}

/* rb -= size followed by an access through rb is one pre-decremented
 * access. */
static bool fuse_pre(RealRegister rd, RealRegister rs, int32 k)
{
    Icode const *n = cg_next_icode;

    RealRegister rv;

    if (n == NULL || rd != rs || k >= 0) return NO;
    if (!is_ldrk_strk(n->op) || mem_packed(n->op)) return NO;
    if (register_number(n->r2.r) != rd || n->r3.i != 0 || -k != mem_size(n->op))
        return NO;
    rv = register_number(n->r1.r);
    if (writes_mem(n->op & J_TABLE_BITS) == 0 && rv == rd) return NO;
    mem_op(n->op & ~J_DEADBITS, rv, rd, 0, 1);
    skip_next = YES;
    return YES;
}

/* ---- block copy and clear ---------------------------------------------- */

/* MOVC: *rd = *rs for n bytes; CLRC: rs is NoRegister.  Both address
 * registers are advanced, which corrupts_r1/r2 report. */
/* MOVC and CLRC.  rd survives (the middle end may still want it), rs does
 * not.  Short blocks are unrolled through at; long ones copy backwards so
 * the loop can end by comparing at with rd, there being no spare register
 * for a count. */
static void block_op(RealRegister rd, RealRegister rs, int32 n, int size)
{
    int32 count = n / size;
    LabelNumber *loop;

    if (count == 0) return;
    out_mov(R_IP, rd);
    if (count > 8) {
        add_integer(R_IP, R_IP, n);
        if (rs != NoRegister) add_integer(rs, rs, n);
    }
    if (rs == NoRegister) out_bitr(R_IR, 3, NO, R_IR);
    if (count <= 8) {
        while (count-- > 0) {
            if (rs != NoRegister) out_mem(NO, R_IR, rs, size, NO, 3);
            out_mem(YES, R_IR, R_IP, size, NO, 3);
        }
        return;
    }
    loop = nextlabel();
    setlabel(loop);
    if (rs != NoRegister) out_mem(NO, R_IR, rs, size, NO, 1);
    out_mem(YES, R_IR, R_IP, size, NO, 1);
    out_cmpr(R_IP, rd);
    branch_to(C_NE, loop);
}

/* ---- case tables ------------------------------------------------------- */

static RealRegister case_reg = NoRegister;
static bool case_reg_dead;

static void casebranch(RealRegister r1, int32 m)
{
    int32 table_bytes = 2 * m + 16;

    if (codep + table_bytes >= mustlitby) dumplits(YES);
    if (codep + table_bytes >= mustbranchby) dump_island();
    if (m - 1 >= -128 && m - 1 <= 127) {
        out_cmpk(r1, m - 1);
    } else {
        load_integer(R_IR, m - 1);
        out_cmpr(r1, R_IR);
    }
    case_reg = r1;
    in_table = YES;
}

/* The first BXX after a CASEBRANCH is the default; the rest form a table
 * of branches indexed by the case register. */
static void case_entry(LabelNumber *dest)
{
    if (dest == RETLAB) {
        dest = returnlab;
        if (!lab_isset_(dest)) return_pending = YES;
    }
    if (case_reg != NoRegister) {
        RealRegister t = case_reg_dead ? case_reg : R_IR;

        branch_to(C_CS, dest);
        if (t != case_reg) out_mov(t, case_reg);
        out_shift(t, NO, YES, NO, 1);
        out_add8(NO, t, 2);
        out_add3(NO, R_PC, t, 0);
        case_reg = NoRegister;
        return;
    }
    branch_to(C_AL, dest);
}

/* ---- the main entry point ---------------------------------------------- */

static int cond_of_q(int32 q)
{
    switch (q & ~Q_UBIT) {
    case Q_EQ & ~Q_UBIT: return C_EQ;
    case Q_NE & ~Q_UBIT: return C_NE;
    case Q_HS & ~Q_UBIT: return C_CS;
    case Q_LO & ~Q_UBIT: return C_CC;
    case Q_MI & ~Q_UBIT: return C_MI;
    case Q_PL & ~Q_UBIT: return C_PL;
    case Q_VS & ~Q_UBIT: return C_VS;
    case Q_VC & ~Q_UBIT: return C_VC;
    case Q_HI & ~Q_UBIT: return C_HI;
    case Q_LS & ~Q_UBIT: return C_LS;
    case Q_GE & ~Q_UBIT: return C_GE;
    case Q_LT & ~Q_UBIT: return C_LT;
    case Q_GT & ~Q_UBIT: return C_GT;
    case Q_LE & ~Q_UBIT: return C_LE;
    case Q_AL & ~Q_UBIT: return C_AL;
    default:
        syserr("condition %lx", (long)q);
        return C_AL;
    }
}

static void return_to(int cond)
{
    if (cond == C_AL) {
        if (!lab_isset_(returnlab)) {
            setlabel(returnlab);
            routine_exit();
        } else {
            branch_to(C_AL, returnlab);
        }
    } else {
        if (!lab_isset_(returnlab)) return_pending = YES;
        branch_to(cond, returnlab);
    }
}

static void shift_op(J_OPCODE op, RealRegister rd, RealRegister rs,
                     int32 m, bool byreg)
{
    bool arith = NO;
    bool left = NO;
    bool rot = NO;

    switch (op & J_TABLE_BITS) {
    case J_SHLK: case J_SHLR: left = YES; break;
    case J_SHRK: case J_SHRR: arith = (op & J_UNSIGNED) == 0; break;
    case J_RORK: case J_RORR: rot = YES; break;
    }
    if (byreg) {
        out_mov(rd, rs);
        out_shiftr(rd, arith, left, rot, (RealRegister)m);
        return;
    }
    m &= 255;
    if (m == 0) {
        out_mov(rd, rs);
        return;
    }
    if (m >= 32 && !rot) {
        if (arith) {
            out_mov(rd, rs);
            out_shift(rd, YES, NO, NO, 31);
        } else {
            load_integer(rd, 0);
        }
        return;
    }
    out_mov(rd, rs);
    out_shift(rd, arith, left, rot, m & 31);
}

/* rd = rs OP rm for a two-address machine. */
static void rr_op(int which, RealRegister rd, RealRegister rs, RealRegister rm)
{
    bool commutative = which != 'S';

    if (rd == rs) {
        /* already in place */
    } else if (rd == rm && commutative) {
        rm = rs;
    } else if (rd == rm) {
        /* rd = rs - rd, via ir */
        out_mov(R_IR, rs);
        out_add3(YES, R_IR, rd, 0);
        out_mov(rd, R_IR);
        return;
    } else {
        out_mov(rd, rs);
    }
    switch (which) {
    case 'A': out_add3(NO, rd, rm, 0); break;
    case 'S': out_add3(YES, rd, rm, 0); break;
    case '&': out_bitr(rd, 1, NO, rm); break;
    case '|': out_bitr(rd, 2, NO, rm); break;
    case '^': out_bitr(rd, 3, NO, rm); break;
    }
}

static void show_instruction_1(const Icode *const ic);

void show_instruction(const Icode *const ic)
{
    J_OPCODE op1 = ic->op & J_TABLE_BITS;

    show_instruction_1(ic);
    note_a3(op1, ic->r1.rr, ic->r2.rr, (int32)ic->r3.i);
}

static void show_instruction_1(const Icode *const ic)
{
    J_OPCODE op = ic->op & ~J_DEADBITS;
    J_OPCODE op1 = op & J_TABLE_BITS;
    RealRegister r1 = ic->r1.rr;
    RealRegister r2 = ic->r2.rr;
    IPtr m = ic->r3.i;          /* a pointer for some opcodes */
    int32 mi = (int32)m;
    RealRegister mr = ic->r3.rr;
    int32 q = op & Q_MASK;

    if (skip_next) {
        skip_next = NO;                 /* already done by the previous one */
        return;
    }
    if (skip_after_next) {
        skip_after_next = NO;
        skip_next = YES;
    }
    if (fwd_from != NoRegister) {
        if (reads_r1(op1) && r1 == fwd_from) r1 = fwd_to;
        if (reads_r2(op1) && r2 == fwd_from) r2 = fwd_to;
        if (reads_r3(op1) && mr == fwd_from) mr = fwd_to;
        fwd_from = NoRegister;
    }
    if (op1 != J_BXX) {
        in_table = NO;
        case_reg = NoRegister;
    }
    if (!in_table && op1 != J_ENDPROC && op1 != J_ENTER) {
        if (codep + 16 >= mustlitby) dumplits(YES);
        if (codep + 16 >= mustbranchby) dump_island();
    }
    switch (op & ~(Q_MASK | J_SIGNED | J_UNSIGNED | J_BASEALIGN4 | J_ALIGNMENT))
    {
case J_ENTER:
        returnlab = nextlabel();
        routine_entry(mi);
        break;
case J_ENDPROC:
        if (!lab_isset_(returnlab) && return_pending) {
            setlabel(returnlab);
            routine_exit();
        }
        if (nfrefs != 0) syserr("unresolved branches at ENDPROC");
        dumplits(NO);
        cnop();
        break;
case J_LABEL:
        setlabel((LabelNumber *)m);
        break;
case J_B:
        if ((LabelNumber *)m == RETLAB)
            return_to(cond_of_q(q));
        else
            branch_to(cond_of_q(q), (LabelNumber *)m);
        break;
case J_CASEBRANCH:
        case_reg_dead = (ic->op & J_DEAD_R1) != 0;
        casebranch(r1, mi);
        break;
case J_BXX:
        case_entry((LabelNumber *)m);
        break;
case J_MOVK:
        load_integer(r1, mi);
        break;
case J_MOVR:
        if (step_after_test(r1, mr)) break;
        if (forward_copy(r1, mr)) break;
        out_mov(r1, mr);
        break;
case J_NEGR:
        if (r1 == mr) {
            out_bitr(r1, 0, NO, r1);
            out_add8(NO, r1, 1);
        } else {
            out_bitr(r1, 3, NO, r1);
            out_add3(YES, r1, mr, 0);
        }
        break;
case J_NOTR:
        out_bitr(r1, 0, NO, mr);
        break;
case J_ADDK:
        if (fuse_pre(r1, r2, mi)) break;
        add_integer(r1, r2, mi);
        break;
case J_SUBK:
        add_integer(r1, r2, -mi);
        break;
case J_RSBK:
        /* r1 = m - r2 */
        if (r1 == r2) {
            load_integer(R_IR, mi);
            out_add3(YES, R_IR, r1, 0);
            out_mov(r1, R_IR);
        } else {
            load_integer(r1, mi);
            out_add3(YES, r1, r2, 0);
        }
        break;
case J_ANDK:
        if (test_bit(r1, r2, mi)) break;
        bit_integer(1, r1, r2, mi);
        break;
case J_ORRK: bit_integer(2, r1, r2, mi); break;
case J_EORK: bit_integer(3, r1, r2, mi); break;
case J_MULK:
        multiply_integer(r1, r2, mi);
        break;
case J_ADDR: rr_op('A', r1, r2, mr); break;
case J_SUBR: rr_op('S', r1, r2, mr); break;
case J_ANDR: rr_op('&', r1, r2, mr); break;
case J_ORRR: rr_op('|', r1, r2, mr); break;
case J_EORR: rr_op('^', r1, r2, mr); break;
case J_RSBR:
        rr_op('S', r1, mr, r2);
        break;
case J_SHLK: case J_SHRK: case J_RORK:
        shift_op(op, r1, r2, mi, NO);
        break;
case J_SHLR: case J_SHRR: case J_RORR:
        shift_op(op, r1, r2, mr, YES);
        break;
case J_CMPK:
        if (mi >= -128 && mi <= 127) {
            out_cmpk(r2, mi);
        } else {
            load_integer(R_IR, mi);
            out_cmpr(r2, R_IR);
        }
        break;
case J_CMPR:
        out_cmpr(r2, mr);
        break;
case J_LDRK: case J_LDRBK: case J_LDRWK:
case J_STRK: case J_STRBK: case J_STRWK:
        if (fuse_post(op, r1, r2, mi)) break;
        mem_op(op, r1, r2, mi, 0);
        break;
case J_LDRR: case J_LDRBR: case J_LDRWR:
case J_STRR: case J_STRBR: case J_STRWR:
        /* base + index: into whichever of them dies here, else into ir */
        {   bool store = writes_mem(op1) != 0;
            RealRegister ra = R_IR;

            if ((ic->op & J_DEAD_R3) && !(store && r1 == mr)) {
                out_add3(NO, mr, r2, 0);
                ra = mr;
            } else if ((ic->op & J_DEAD_R2) && !(store && r1 == r2)) {
                out_add3(NO, r2, mr, 0);
                ra = r2;
            } else {
                out_mov(R_IR, r2);
                out_add3(NO, R_IR, mr, 0);
            }
            mem_op(op, r1, ra, 0, 0);
        }
        break;
case J_CALLK:
        if (!inline_ll((Symstr *)m)) call_k((Symstr *)m);
        break;
case J_CALLR:
        call_r(mr);
        break;
case J_OPSYSK:
        cc_err(gen_err_swi);
        break;
case J_ADCON:
        load_adcon(r1, (Symstr *)m, (int32)r2);
        break;
case J_ADCONF:
case J_ADCOND:
        load_fp_adcon(r1, op, (FloatCon *)m);
        break;
case J_ADCONLL:
        load_ll_adcon(r1, (Int64Con *)m);
        break;
case J_STRING:
        load_string(r1, (StringSegList *)m, (int32)r2 > 0 ? (int32)r2 : 0);
        break;
case J_WORD:
        outHW((unsigned32)mi);
        break;
case J_MOVC:
        block_op(r1, r2, mi, (op & J_ALIGNMENT) >= J_ALIGN4 ? 4 : 1);
        break;
case J_CLRC:
        block_op(r1, NoRegister, mi, (op & J_ALIGNMENT) >= J_ALIGN4 ? 4 : 1);
        break;
case J_PUSHM:
        syserr("PUSHM with a fixed frame");
        break;
case J_STACK:
case J_SETSP:
case J_USE:
case J_VSTORE:
case J_INIT:
case J_INITF:
case J_INITD:
case J_INFOLINE:
case J_INFOSCOPE:
case J_INFOBODY:
case J_COUNT:
case J_ORG:
case J_RESULT2:
        break;
default:
        syserr(syserr_show_inst_dir, (long)op);
        break;
    }
}

void branch_round_literals(LabelNumber *l)
{
    branch_to(C_AL, l);
}

void mcdep_init(void)
{
    codebuf_reinit2();
    avoidallocating(R_LR);
    mustlitby = 0x10000000;
    mustbranchby = 0x10000000;
    current_procnum = 0;
    label_values = NULL;
    label_references = NULL;
    frefs = NULL;
    nfrefs = 0;
}

void localcg_reinit(void)
{
    mustlitby = 0x10000000;
    mustbranchby = 0x10000000;
    in_table = NO;
    case_reg = NoRegister;
    frefs = NULL;
    nfrefs = 0;
}

void localcg_tidy(void)
{
}

void localcg_newliteralpool(void)
{
    mustlitby = 0x10000000;
}

/* end of meow/gen.c */
