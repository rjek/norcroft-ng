/*
 * x86/gen.c -- local code generator for the x86 back end.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * show_instruction() turns each (register-allocated) jopcode into one or
 * more x86 instructions, which are kept as a structured list for the
 * current function.  asm.c prints the list as GNU assembler (AT&T) text
 * when the function is complete.  The same list could later feed a
 * binary encoder.
 *
 * Frame layout (i386, frame pointer always used):
 *
 *      ebp+8+n     incoming argument byte n
 *      ebp+4       return address
 *      ebp+0       saved ebp
 *      ebp-4*k     k saved callee-save registers (ebx, esi, edi)
 *      ...-16      16-byte scratch slot (x87 <-> SSE transfers etc.)
 *      ...         padding to keep esp 16-byte aligned at calls
 *      ...-p       local at BINDADDR_LOC p
 *      esp+n       outgoing argument byte n (TARGET_STACK_MOVES_ONCE)
 */

#include <string.h>
#include <stdlib.h>

#include "globals.h"
#include "mcdep.h"
#include "mcdpriv.h"
#include "xrefs.h"
#include "jopcode.h"
#include "store.h"
#include "codebuf.h"
#include "regalloc.h"
#include "cg.h"
#include "flowgraf.h"
#include "builtin.h"
#include "bind.h"
#include "vargen.h"
#include "simplify.h"   /* MCR_SORT_xxx */
#include "errors.h"

#include "x86ins.h"

/* ---------------------------------------------------------------- */
/* Registers                                                          */
/* ---------------------------------------------------------------- */

/* Internal register number to hardware encoding (see target.h).      */
static int const hwreg[NINTREGS] = {
    X86_EAX, X86_EDX, X86_ECX, X86_EBX, X86_ESI, X86_EDI, X86_EBP, X86_ESP,
    -1
};

#define isfpreg(r) ((r) >= R_F0 && (r) < R_F0+NFLTREGS)
#define xmm(r)     ((int)((r) - R_F0))

/* Hardware registers with 8-bit subregisters (al, cl, dl, bl).       */
#define byteable(hw) ((hw) <= X86_EBX)

static int hw(RealRegister r)
{   if (r < 0 || r >= NINTREGS || hwreg[r] < 0) syserr("x86 hw reg %ld", (long)r);
    return hwreg[r];
}

/* ---------------------------------------------------------------- */
/* The instruction list                                               */
/* ---------------------------------------------------------------- */

X86Ins *x86_insns, *x86_insns_tail;
int32 x86_fnlabel;            /* distinguishes labels between functions */

static X86Ins *newins(char const *mnem)
{   X86Ins *p = (X86Ins *)SynAlloc(sizeof(X86Ins));
    memclr(p, sizeof(X86Ins));
    p->mnem = mnem;
    if (x86_insns_tail == NULL) x86_insns = p; else x86_insns_tail->next = p;
    x86_insns_tail = p;
    return p;
}

static X86Op op_reg(int hwr, int size)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_REG, o.reg = hwr, o.size = size;
    return o;
}

static X86Op op_xmm(int n)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_XMM, o.reg = n;
    return o;
}

static X86Op op_imm(int32 n)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_IMM, o.disp = n;
    return o;
}

/* $sym+n */
static X86Op op_symimm(Symstr const *sym, int32 n)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_IMM, o.sym = sym, o.disp = n;
    return o;
}

static X86Op op_mem(int base, int index, int scale, int32 disp)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_MEM, o.reg = base, o.index = index, o.scale = scale;
    o.disp = disp;
    return o;
}

static X86Op op_lab(int32 lab)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_LAB, o.disp = lab;
    return o;
}

static X86Op op_sym(Symstr const *sym)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_SYM, o.sym = sym;
    return o;
}

static X86Op op_star(X86Op o)
{   o.star = 1;
    return o;
}

static X86Ins *ins0(char const *m)
{   return newins(m);
}

static X86Ins *ins1(char const *m, X86Op a)
{   X86Ins *p = newins(m);
    p->nops = 1, p->op[0] = a;
    return p;
}

/* AT&T operand order: source first. */
static X86Ins *ins2(char const *m, X86Op src, X86Op dst)
{   X86Ins *p = newins(m);
    p->nops = 2, p->op[0] = src, p->op[1] = dst;
    return p;
}

static X86Ins *ins3(char const *m, X86Op a, X86Op b, X86Op c)
{   X86Ins *p = newins(m);
    p->nops = 3, p->op[0] = a, p->op[1] = b, p->op[2] = c;
    return p;
}

static void deflabel(int32 lab)
{   X86Ins *p = newins(NULL);
    p->kind = XI_LABEL, p->op[0] = op_lab(lab);
}

static void directive(char const *text)
{   X86Ins *p = newins(text);
    p->kind = XI_DIRECTIVE;
}

static X86Op R(RealRegister r) { return op_reg(hw(r), 4); }
static X86Op X(RealRegister r) { return op_xmm(xmm(r)); }

/* ---------------------------------------------------------------- */
/* Labels                                                             */
/* ---------------------------------------------------------------- */

/* Local labels are .L<fn>_<n>.  Compiler-generated labels use their  */
/* LabelNumber name; labels private to gen.c are allocated downwards  */
/* from -1 so they can't clash.                                       */
static int32 privatelabels;     /* the last one allocated          */
#define LAB_RET (-1)

static bool retlab_used;       /* something refers to the epilogue label */

static int32 labnum(LabelNumber *l)
{   if (l == RETLAB) {
        retlab_used = YES;
        return LAB_RET;
    }
    if (is_exit_label(l)) syserr("x86: odd exit label");
    return lab_name_(l) & 0x7fffffff;
}

static int32 newlabel(void)
{   return --privatelabels;
}

/* ---------------------------------------------------------------- */
/* The frame                                                          */
/* ---------------------------------------------------------------- */

static int32 nsaved;           /* number of callee-save regs pushed      */
static int32 framepad;         /* bytes of alignment padding              */

static int32 const savedregs[] = { 3, 4, 5 };   /* ebx esi edi */
#define NSAVEDREGS 3

/* Offset from ebp of the scratch slot, and its size.                  */
#define SCRATCHSIZE 16
#define scratch_offset() (-4*nsaved - SCRATCHSIZE)

RealRegister local_base(Binder const *b)
{   IGNORE(b);
    return R_FP;
}

int32 local_address(Binder const *b)
{   int32 p = bindaddr_(b);
    int32 q = p & ~BINDADDR_MASK;
    switch (p & BINDADDR_MASK)
    {
default:
        syserr(syserr_local_addr, (long)p);
case BINDADDR_LOC:
        return -4*nsaved - SCRATCHSIZE - framepad - q;
case BINDADDR_ARG:
        return 8 + q;
    }
}

static bool function_saves(int32 r)
{   return member_RealRegSet(&regmaskvec, r);
}

static void gen_prologue(void)
{   int32 i, align;
    ins1("pushl", op_reg(X86_EBP, 4));
    ins2("movl", op_reg(X86_ESP, 4), op_reg(X86_EBP, 4));
    nsaved = 0;
    for (i = 0; i < NSAVEDREGS; i++)
        if (function_saves(savedregs[i])) {
            ins1("pushl", R(savedregs[i]));
            nsaved++;
        }
    /* On entry esp = 12 (mod 16); after pushing ebp and the saved     */
    /* registers it is 8-4*nsaved.  Choose the padding so that esp is  */
    /* 16-byte aligned once the scratch slot, padding and              */
    /* greatest_stackdepth bytes of locals/outgoing args are dropped.  */
    align = (8 - 4*nsaved - SCRATCHSIZE - greatest_stackdepth) & 15;
    framepad = align;
    {   int32 frame = SCRATCHSIZE + framepad + greatest_stackdepth;
        ins2("subl", op_imm(frame), op_reg(X86_ESP, 4));
    }
}

static bool returns_struct_in_memory(void)
{   return (currentfunction.resultrep & MCR_SORT_MASK) == MCR_SORT_STRUCT &&
           currentfunction.nresultregs == 0;
}

static void gen_epilogue(void)
{   int32 i;
    int32 rep = currentfunction.resultrep;
    /* The ABI returns floating results in st(0).                     */
    if (rep == MCR_SORT_FLOATING+4) {
        ins2("movss", op_xmm(0), op_mem(X86_EBP, -1, 0, scratch_offset()));
        ins1("flds", op_mem(X86_EBP, -1, 0, scratch_offset()));
    } else if (rep == MCR_SORT_FLOATING+8) {
        ins2("movsd", op_xmm(0), op_mem(X86_EBP, -1, 0, scratch_offset()));
        ins1("fldl", op_mem(X86_EBP, -1, 0, scratch_offset()));
    } else if (returns_struct_in_memory()) {
        /* ... and the address of a memory struct result in eax.      */
        ins2("movl", op_mem(X86_EBP, -1, 0, 8), op_reg(X86_EAX, 4));
    }
    ins2("leal", op_mem(X86_EBP, -1, 0, -4*nsaved), op_reg(X86_ESP, 4));
    for (i = NSAVEDREGS; --i >= 0; )
        if (function_saves(savedregs[i]))
            ins1("popl", R(savedregs[i]));
    ins1("popl", op_reg(X86_EBP, 4));
    /* The callee pops the hidden struct-result pointer.              */
    if (returns_struct_in_memory())
        ins1("ret", op_imm(4));
    else
        ins0("ret");
}

/* ---------------------------------------------------------------- */
/* Conditions                                                         */
/* ---------------------------------------------------------------- */

/* State of the flags set by the last compare, for the branch that    */
/* follows it.                                                        */
static bool cmp_is_fp;          /* set by ucomiss/ucomisd             */
static bool cmp_fp_swapped;     /* operands were swapped (see below)  */

static char const *jcc_int(int32 q)
{   switch (q)
    {
case Q_EQ: case Q_UEQ:  return "je";
case Q_NE: case Q_UNE:  return "jne";
case Q_GT:              return "jg";
case Q_GE:              return "jge";
case Q_LT:              return "jl";
case Q_LE:              return "jle";
case Q_HI:              return "ja";
case Q_HS:              return "jae";
case Q_LO:              return "jb";
case Q_LS:              return "jbe";
case Q_MI:              return "js";
case Q_PL:              return "jns";
default:                syserr("x86: condition %lx", (long)q);
                        return "jmp";
    }
}

/*
 * Floating compares use ucomis[sd], which sets the flags like an
 * unsigned integer compare, and sets ZF, PF and CF if unordered.  So
 * that ordered comparisons are false for NaN, "a < b" is done as "b > a"
 * (the operands are swapped at the compare), and "above" conditions are
 * used.  The negations of those conditions (which flowgraf may choose to
 * branch on) then come out true for NaN, as they should.
 */
static void branch_fp(int32 q, int32 lab)
{   switch (q & ~Q_UBIT)
    {
case Q_EQ & ~Q_UBIT:
        {   int32 skip = newlabel();
            ins1("jp", op_lab(skip));
            ins1("je", op_lab(lab));
            deflabel(skip);
            return;
        }
case Q_NE & ~Q_UBIT:
        ins1("jne", op_lab(lab));
        ins1("jp", op_lab(lab));
        return;
case Q_GT: ins1(cmp_fp_swapped ? "jb" : "ja", op_lab(lab));   return;
case Q_GE: ins1(cmp_fp_swapped ? "jbe" : "jae", op_lab(lab)); return;
case Q_LT: ins1(cmp_fp_swapped ? "ja" : "jb", op_lab(lab));   return;
case Q_LE: ins1(cmp_fp_swapped ? "jae" : "jbe", op_lab(lab)); return;
default:
        syserr("x86: fp condition %lx", (long)q);
    }
}

static void branch(int32 q, int32 lab)
{   if (q == Q_AL) {
        ins1("jmp", op_lab(lab));
        return;
    }
    if (q == Q_NOT) return;
    if (cmp_is_fp) branch_fp(q, lab);
    else ins1(jcc_int(q), op_lab(lab));
}

/* ---------------------------------------------------------------- */
/* Helpers                                                            */
/* ---------------------------------------------------------------- */

/* The memory operand of a load/store: [r2+m] or [r2+r3].             */
static X86Op memop(Icode const *ic, bool rr)
{   if (rr) return op_mem(hw(ic->r2.rr), hw(ic->r3.rr), 1, 0);
    return op_mem(hw(ic->r2.rr), -1, 0, ic->r3.i);
}

static void movr(RealRegister to, RealRegister from)
{   if (to != from) ins2("movl", R(from), R(to));
}

static void movx(RealRegister to, RealRegister from)
{   if (to != from) ins2("movaps", X(from), X(to));
}

/* r1 = r2 op r3, op commutative ("addl" etc).                        */
static void commutative_rr(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 == r2) ins2(m, R(r3), R(r1));
    else if (r1 == r3) ins2(m, R(r2), R(r1));
    else { movr(r1, r2); ins2(m, R(r3), R(r1)); }
}

/* r1 = r2 op r3, op not commutative.  regalloc ensures r1 != r3      */
/* unless r1 == r2 (TARGET_HAS_2ADDRESS_CODE).                        */
static void asym_rr(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 != r2 && r1 == r3) syserr("x86: 2-address clash");
    movr(r1, r2);
    ins2(m, R(r3), R(r1));
}

/* r1 = r2 op k.                                                      */
static void op_rk(char const *m, Icode const *ic)
{   movr(ic->r1.rr, ic->r2.rr);
    ins2(m, op_imm(ic->r3.i), R(ic->r1.rr));
}

/* Floating point equivalents (m is the SSE mnemonic).                */
static void fcommutative_rr(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 == r2) ins2(m, X(r3), X(r1));
    else if (r1 == r3) ins2(m, X(r2), X(r1));
    else { movx(r1, r2); ins2(m, X(r3), X(r1)); }
}

static void fasym_rr(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 != r2 && r1 == r3) syserr("x86: 2-address clash (fp)");
    movx(r1, r2);
    ins2(m, X(r3), X(r1));
}

/* r1 = r3 op r2 (the reverse forms).                                 */
static void fasym_rev(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 == r3) ins2(m, X(r2), X(r1));
    else if (r1 == r2) {
        /* r1 = r3 op r1: go via the scratch slot.                    */
        X86Op s = op_mem(X86_EBP, -1, 0, scratch_offset());
        ins2(m[3] == 's' ? "movss" : "movsd", X(r1), s);
        movx(r1, r3);
        ins2(m, s, X(r1));
    } else { movx(r1, r3); ins2(m, X(r2), X(r1)); }
}

/* A floating constant: put it in the read-only data area.            */
static void load_fpconst(RealRegister r1, FloatCon *fc, int32 len)
{   int32 off;
    DataAreaSort old = SetDataArea(DS_Const);
    padstatic(len);
    off = constdata_size();
    gendcE(len, fc);
    SetDataArea(old);
    ins2(len == 4 ? "movss" : "movsd",
         op_mem(-1, -1, 0, off), X(r1))->op[0].sym = bindsym_(constdatasegment);
}

/* Constants emitted by asm.c if used.                                */
bool x86_negmask_used;          /* sign-bit masks for negating floats  */
bool x86_two32_used;            /* 2^32 as a double                    */

/* ---------------------------------------------------------------- */
/* Switch tables                                                      */
/* ---------------------------------------------------------------- */

static int32 casetab_label, casedef_label, casetab_entries;
static LabelNumber *casetab_default;

static void casetab_entry(int32 lab)
{   X86Ins *p = ins1(".long", op_lab(lab));
    p->kind = XI_DATA;
}

/* Finish a switch table: the default entry (entry 0) is only reached   */
/* via a jump from the out-of-range check.                              */
static void close_casetable(void)
{   directive("\t.text");
    deflabel(casedef_label);
    ins1("jmp", op_lab(labnum(casetab_default)));
}

/* Complete a table whose remaining entries are for the next code.      */
static void end_casetable(void)
{   int32 next = newlabel();
    if (casetab_default == NULL) syserr("x86: empty case table");
    while (casetab_entries-- > 0) casetab_entry(next);
    casetab_entries = 0;
    close_casetable();
    deflabel(next);
}

/* ---------------------------------------------------------------- */
/* The instruction selector                                           */
/* ---------------------------------------------------------------- */

void show_instruction(Icode const *const ic)
{
    J_OPCODE op = ic->op;
    J_OPCODE opm = op & J_TABLE_BITS;
    RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    int32 m = ic->r3.i;

    if (debugging(DEBUG_CG)) print_jopcode(ic);

    /* flowgraf omits trailing table entries that would branch to the  */
    /* next instruction (as they may simply fall through on ARM).       */
    if (casetab_entries > 0 && opm != J_BXX)
        end_casetable();

    switch (opm)
    {
    case J_NOOP: case J_INIT: case J_INITF: case J_INITD:
    case J_USE: case J_USEF: case J_USED: case J_VSTORE:
    case J_INFOLINE: case J_INFOSCOPE: case J_INFOBODY:
    case J_STACK: case J_SETSP: case J_COUNT:
        return;

    case J_ENTER:
        gen_prologue();
        return;

    case J_ENDPROC:
        if (retlab_used) {
            deflabel(LAB_RET);
            gen_epilogue();
        }
        return;

    case J_LABEL:
        deflabel(labnum(ic->r3.l));
        return;

    case J_B:
        if (ic->r3.l == RETLAB && (op & Q_MASK) == Q_AL) {
            gen_epilogue();
            return;
        }
        branch(op & Q_MASK, labnum(ic->r3.l));
        return;

    case J_CASEBRANCH:
        /* r1 is the index, m the table size.  Entry 0 of the table   */
        /* (the first J_BXX) is the default, for out-of-range values. */
        casetab_label = newlabel();
        casedef_label = newlabel();
        casetab_entries = m;
        casetab_default = NULL;
        ins2("cmpl", op_imm(m-1), R(r1));
        ins1("jae", op_lab(casedef_label));
        {   X86Op t = op_mem(-1, hw(r1), 4, 4);
            t.lab = casetab_label, t.haslab = 1;
            ins1("jmp", op_star(t));
        }
        directive("\t.section\t.rodata");
        directive("\t.p2align\t2");
        deflabel(casetab_label);
        cmp_is_fp = NO;
        return;

    case J_BXX:
        if (casetab_entries <= 0) syserr("x86: stray BXX");
        if (casetab_default == NULL) casetab_default = ic->r3.l;
        casetab_entry(labnum(ic->r3.l));
        if (--casetab_entries == 0) close_casetable();
        return;

    /* ---- moves and constants ---- */
    case J_MOVR:
        movr(r1, r3);
        return;
    case J_MOVK:
        ins2("movl", op_imm(m), R(r1));
        return;
    case J_ADCON:
        ins2("movl", op_symimm(ic->r3.sym, ic->r2.i), R(r1));
        return;
    case J_ADCONLL:
        /* The address of a long long literal.                        */
        {   int32 off;
            DataAreaSort old = SetDataArea(DS_Const);
            padstatic(8);
            off = constdata_size();
            gendcI(4, (int32)ic->r3.i64->bin.i.lo);
            gendcI(4, (int32)ic->r3.i64->bin.i.hi);
            SetDataArea(old);
            ins2("movl", op_symimm(bindsym_(constdatasegment), off), R(r1));
        }
        return;
    case J_STRING:
        {   int32 off;
            DataAreaSort old = SetDataArea(DS_Const);
            off = constdata_size();
            vg_genstring(ic->r3.s, stringlength(ic->r3.s)+1, 0);
            padstatic(4);
            SetDataArea(old);
            ins2("movl", op_symimm(bindsym_(constdatasegment), off), R(r1));
        }
        return;

    /* ---- integer loads and stores ---- */
    case J_LDRK: case J_LDRR:
        ins2("movl", memop(ic, opm == J_LDRR), R(r1));
        return;
    case J_LDRBK: case J_LDRBR:
        ins2(op & J_SIGNED ? "movsbl" : "movzbl", memop(ic, opm == J_LDRBR), R(r1));
        return;
    case J_LDRWK: case J_LDRWR:
        ins2(op & J_SIGNED ? "movswl" : "movzwl", memop(ic, opm == J_LDRWR), R(r1));
        return;
    case J_STRK: case J_STRR:
        ins2("movl", R(r1), memop(ic, opm == J_STRR));
        return;
    case J_STRWK: case J_STRWR:
        ins2("movw", op_reg(hw(r1), 2), memop(ic, opm == J_STRWR));
        return;
    case J_STRBK: case J_STRBR:
        {   X86Op mem = memop(ic, opm == J_STRBR);
            if (byteable(hw(r1)))
                ins2("movb", op_reg(hw(r1), 1), mem);
            else {
                /* esi/edi have no byte form: borrow a byteable       */
                /* register not used in the address.                  */
                int t;
                for (t = X86_EAX; t <= X86_EBX; t++)
                    if (t != mem.reg && t != mem.index) break;
                ins1("pushl", op_reg(t, 4));
                ins2("movl", R(r1), op_reg(t, 4));
                ins2("movb", op_reg(t, 1), mem);
                ins1("popl", op_reg(t, 4));
            }
        }
        return;

    /* ---- integer arithmetic ---- */
    case J_ADDK:
        if (r1 == r2) ins2("addl", op_imm(m), R(r1));
        else ins2("leal", op_mem(hw(r2), -1, 0, m), R(r1));
        return;
    case J_ADDR:
        if (r1 != r2 && r1 != r3)
            ins2("leal", op_mem(hw(r2), hw(r3), 1, 0), R(r1));
        else commutative_rr("addl", ic);
        return;
    case J_SUBK:
        if (r1 == r2) ins2("subl", op_imm(m), R(r1));
        else ins2("leal", op_mem(hw(r2), -1, 0, -m), R(r1));
        return;
    case J_SUBR:
        asym_rr("subl", ic);
        return;
    case J_RSBK:                        /* r1 = k - r2 */
        if (r1 == r2) {
            ins1("negl", R(r1));
            ins2("addl", op_imm(m), R(r1));
        } else {
            ins2("movl", op_imm(m), R(r1));
            ins2("subl", R(r2), R(r1));
        }
        return;
    case J_RSBR:                        /* r1 = r3 - r2 */
        if (r1 == r3) ins2("subl", R(r2), R(r1));
        else if (r1 == r2) {
            ins1("negl", R(r1));
            ins2("addl", R(r3), R(r1));
        } else {
            movr(r1, r3);
            ins2("subl", R(r2), R(r1));
        }
        return;
    case J_ANDK: op_rk("andl", ic); return;
    case J_ORRK: op_rk("orl", ic);  return;
    case J_EORK: op_rk("xorl", ic); return;
    case J_ANDR: commutative_rr("andl", ic); return;
    case J_ORRR: commutative_rr("orl", ic);  return;
    case J_EORR: commutative_rr("xorl", ic); return;
    case J_MULK:
        ins3("imull", op_imm(m), R(r2), R(r1));
        return;
    case J_MULR:
        commutative_rr("imull", ic);
        return;
    case J_DIVR: case J_REMR:
        /* RealRegisterUse keeps r1, r2 and r3 out of eax and edx.    */
        ins2("movl", R(r2), op_reg(X86_EAX, 4));
        if (op & J_UNSIGNED) {
            ins2("xorl", op_reg(X86_EDX, 4), op_reg(X86_EDX, 4));
            ins1("divl", R(r3));
        } else {
            ins0("cltd");
            ins1("idivl", R(r3));
        }
        ins2("movl", op_reg(opm == J_DIVR ? X86_EAX : X86_EDX, 4), R(r1));
        return;
    case J_SHLK:
        op_rk("shll", ic);
        return;
    case J_SHRK:
        op_rk(op & J_UNSIGNED ? "shrl" : "sarl", ic);
        return;
    case J_SHLR: case J_SHRR:
        /* RealRegisterUse keeps r1 and r2 out of ecx.                */
        ins2("movl", R(r3), op_reg(X86_ECX, 4));
        movr(r1, r2);
        ins2(opm == J_SHLR ? "shll" : op & J_UNSIGNED ? "shrl" : "sarl",
             op_reg(X86_ECX, 1), R(r1));
        return;
    case J_NEGR:
        movr(r1, r3);
        ins1("negl", R(r1));
        return;
    case J_NOTR:
        movr(r1, r3);
        ins1("notl", R(r1));
        return;
    case J_EXTEND:
        /* r3: 0 or 1 = from byte, 2 = from halfword.                 */
        if (m == 2) ins2("movswl", op_reg(hw(r2), 2), R(r1));
        else if (byteable(hw(r2))) ins2("movsbl", op_reg(hw(r2), 1), R(r1));
        else {
            movr(r1, r2);
            ins2("shll", op_imm(24), R(r1));
            ins2("sarl", op_imm(24), R(r1));
        }
        return;

    /* ---- compares ---- */
    case J_CMPK:
        if (m == 0) ins2("testl", R(r2), R(r2));
        else ins2("cmpl", op_imm(m), R(r2));
        cmp_is_fp = NO;
        return;
    case J_CMPR:
        ins2("cmpl", R(r3), R(r2));
        cmp_is_fp = NO;
        return;
    case J_CMPFR: case J_CMPDR:
        {   int32 q = op & Q_MASK & ~Q_UBIT;
            char const *mn = opm == J_CMPFR ? "ucomiss" : "ucomisd";
            cmp_is_fp = YES;
            cmp_fp_swapped = (q == Q_LT || q == Q_LE);
            if (cmp_fp_swapped) ins2(mn, X(r2), X(r3));
            else ins2(mn, X(r3), X(r2));
        }
        return;

    /* ---- calls ---- */
    case J_CALLK: case J_CALLR:
        {   int32 desc = ic->r2.i;
            if (opm == J_CALLK) {
                obj_symref(ic->r3.sym, xr_code, 0);
                ins1("call", op_sym(ic->r3.sym));
            } else
                ins1("call", op_star(R(r3)));
            if (desc & (K_FLTRESULT|K_DBLRESULT)) {
                /* Move the result from st(0) to xmm0.                */
                X86Op s = op_mem(X86_EBP, -1, 0, scratch_offset());
                bool d = (desc & K_DBLRESULT) != 0;
                ins1(d ? "fstpl" : "fstps", s);
                ins2(d ? "movsd" : "movss", s, op_xmm(0));
            }
            /* A memory struct result's pointer is popped by callee.  */
            if (desc & K_STRUCTRESULT)
                ins2("subl", op_imm(4), op_reg(X86_ESP, 4));
        }
        return;

    /* ---- block moves (r1 = dest, r2 = source, r3 = byte count) ---- */
    case J_MOVC:
        ins1("pushl", op_reg(X86_ESI, 4));
        ins1("pushl", op_reg(X86_EDI, 4));
        ins1("pushl", op_reg(X86_ECX, 4));
        ins1("pushl", R(r2));
        ins1("pushl", R(r1));
        ins1("popl", op_reg(X86_EDI, 4));
        ins1("popl", op_reg(X86_ESI, 4));
        if ((m & 3) == 0) {
            ins2("movl", op_imm(m >> 2), op_reg(X86_ECX, 4));
            ins0("rep movsl");
        } else {
            ins2("movl", op_imm(m), op_reg(X86_ECX, 4));
            ins0("rep movsb");
        }
        ins1("popl", op_reg(X86_ECX, 4));
        ins1("popl", op_reg(X86_EDI, 4));
        ins1("popl", op_reg(X86_ESI, 4));
        return;
    case J_CLRC:
        ins1("pushl", op_reg(X86_EDI, 4));
        ins1("pushl", op_reg(X86_ECX, 4));
        ins1("pushl", op_reg(X86_EAX, 4));
        ins1("pushl", R(r1));
        ins1("popl", op_reg(X86_EDI, 4));
        ins2("xorl", op_reg(X86_EAX, 4), op_reg(X86_EAX, 4));
        if ((m & 3) == 0) {
            ins2("movl", op_imm(m >> 2), op_reg(X86_ECX, 4));
            ins0("rep stosl");
        } else {
            ins2("movl", op_imm(m), op_reg(X86_ECX, 4));
            ins0("rep stosb");
        }
        ins1("popl", op_reg(X86_EAX, 4));
        ins1("popl", op_reg(X86_ECX, 4));
        ins1("popl", op_reg(X86_EDI, 4));
        return;

    /* ---- floating point (SSE2) ---- */
    case J_LDRFK: case J_LDRFR:
        ins2("movss", memop(ic, opm == J_LDRFR), X(r1));
        return;
    case J_LDRDK: case J_LDRDR:
        ins2("movsd", memop(ic, opm == J_LDRDR), X(r1));
        return;
    case J_STRFK: case J_STRFR:
        ins2("movss", X(r1), memop(ic, opm == J_STRFR));
        return;
    case J_STRDK: case J_STRDR:
        ins2("movsd", X(r1), memop(ic, opm == J_STRDR));
        return;
    case J_MOVFR: case J_MOVDR:
        movx(r1, r3);
        return;
    case J_MOVFK:
        load_fpconst(r1, ic->r3.f, 4);
        return;
    case J_MOVDK:
        load_fpconst(r1, ic->r3.f, 8);
        return;
    case J_ADDFR: fcommutative_rr("addss", ic); return;
    case J_ADDDR: fcommutative_rr("addsd", ic); return;
    case J_MULFR: fcommutative_rr("mulss", ic); return;
    case J_MULDR: fcommutative_rr("mulsd", ic); return;
    case J_SUBFR: fasym_rr("subss", ic); return;
    case J_SUBDR: fasym_rr("subsd", ic); return;
    case J_DIVFR: fasym_rr("divss", ic); return;
    case J_DIVDR: fasym_rr("divsd", ic); return;
    case J_RSBFR: fasym_rev("subss", ic); return;
    case J_RSBDR: fasym_rev("subsd", ic); return;
    case J_RDVFR: fasym_rev("divss", ic); return;
    case J_RDVDR: fasym_rev("divsd", ic); return;
    case J_NEGFR: case J_NEGDR:
        {   X86Op mask = op_mem(-1, -1, 0, 0);
            mask.lab = opm == J_NEGFR ? XLAB_NEGMASKF : XLAB_NEGMASKD;
            mask.haslab = 1;
            x86_negmask_used = YES;
            movx(r1, r3);
            ins2(opm == J_NEGFR ? "xorps" : "xorpd", mask, X(r1));
        }
        return;
    case J_FLTFR: case J_FLTDR:
        if (op & J_UNSIGNED) {
            /* Convert as signed, then add 2^32 if the top bit was set; */
            /* for float, do that in double to avoid double rounding.   */
            int32 skip = newlabel();
            ins2("cvtsi2sd", R(r3), X(r1));
            ins2("testl", R(r3), R(r3));
            ins1("jns", op_lab(skip));
            {   X86Op two32 = op_mem(-1, -1, 0, 0);
                two32.lab = XLAB_TWO32, two32.haslab = 1;
                x86_two32_used = YES;
                ins2("addsd", two32, X(r1));
            }
            deflabel(skip);
            if (opm == J_FLTFR) ins2("cvtsd2ss", X(r1), X(r1));
        } else
            ins2(opm == J_FLTFR ? "cvtsi2ss" : "cvtsi2sd", R(r3), X(r1));
        return;
    case J_FIXFR: case J_FIXDR:
        if (op & J_UNSIGNED) {
            /* Use the x87 to convert to a 64-bit integer, truncating,  */
            /* and take the low word.                                   */
            int32 s = scratch_offset();
            X86Op cw_old = op_mem(X86_EBP, -1, 0, s + 8);
            X86Op cw_new = op_mem(X86_EBP, -1, 0, s + 10);
            X86Op val = op_mem(X86_EBP, -1, 0, s);
            ins2(opm == J_FIXFR ? "movss" : "movsd", X(r3), val);
            ins1(opm == J_FIXFR ? "flds" : "fldl", val);
            ins1("fnstcw", cw_old);
            ins2("movzwl", cw_old, R(r1));
            ins2("orl", op_imm(0x0c00), R(r1));    /* round to zero    */
            ins2("movw", op_reg(hw(r1), 2), cw_new);
            ins1("fldcw", cw_new);
            ins1("fistpll", val);
            ins1("fldcw", cw_old);
            ins2("movl", val, R(r1));
        } else
            ins2(opm == J_FIXFR ? "cvttss2si" : "cvttsd2si", X(r3), R(r1));
        return;
    case J_MOVIDR:                      /* r2 = low word, r3 = high word */
        {   int32 s = scratch_offset();
            ins2("movl", R(r2), op_mem(X86_EBP, -1, 0, s));
            ins2("movl", R(r3), op_mem(X86_EBP, -1, 0, s + 4));
            ins2("movsd", op_mem(X86_EBP, -1, 0, s), X(r1));
        }
        return;
    case J_MOVDIR:                      /* r1 = low word, r2 = high word */
        {   int32 s = scratch_offset();
            ins2("movsd", X(r3), op_mem(X86_EBP, -1, 0, s));
            ins2("movl", op_mem(X86_EBP, -1, 0, s), R(r1));
            ins2("movl", op_mem(X86_EBP, -1, 0, s + 4), R(r2));
        }
        return;
    case J_MOVFDR:                      /* float -> double */
        ins2("cvtss2sd", X(r3), X(r1));
        return;
    case J_MOVDFR:                      /* double -> float */
        ins2("cvtsd2ss", X(r3), X(r1));
        return;
    case J_MOVIFR:                      /* int bits -> float reg */
        ins2("movd", R(r3), X(r1));
        return;
    case J_MOVFIR:                      /* float bits -> int reg */
        ins2("movd", X(r3), R(r1));
        return;
    }
    syserr("x86: unsupported jopcode %ld (%s)", (long)opm,
#if defined ENABLE_CG || defined ENABLE_REGS || defined ENABLE_CSE
           joptable[opm].name
#else
           "?"
#endif
           );
}

/* ---------------------------------------------------------------- */
/* Other interface functions                                          */
/* ---------------------------------------------------------------- */

void localcg_reinit(void)
{   x86_insns = x86_insns_tail = NULL;
    privatelabels = LAB_RET;
    retlab_used = NO;
    casetab_entries = 0;
    cmp_is_fp = NO;
    nsaved = framepad = 0;
    x86_fnlabel++;
}

void localcg_tidy(void)
{
}

void mcdep_init(void)
{   x86_negmask_used = NO;
    x86_two32_used = NO;
}

void setlabel(LabelNumber *l)
{   IGNORE(l);
}

void branch_round_literals(LabelNumber *m)
{   IGNORE(m);
}

/* end of x86/gen.c */
