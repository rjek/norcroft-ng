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
 *
 * Frame layout (x86-64):
 *
 *      rbp+16+n    incoming stack argument byte n
 *      rbp+8       return address
 *      rbp+0       saved rbp
 *      rbp-8*k     k saved callee-save registers (rbx, r12-r15)
 *      ...-16      16-byte scratch slot; its second half holds the
 *                  address of a struct result returned in memory
 *      ...         home area for the register arguments (see below)
 *      ...         padding to keep rsp 16-byte aligned at calls
 *      ...-p       local at BINDADDR_LOC p
 *      rsp+n       outgoing stack argument byte n
 *
 * The home area gives the arguments passed in registers an address.  For
 * a variadic function it is the psABI's register save area: rdi, rsi,
 * rdx, rcx, r8, r9, then xmm0-7 in 16 bytes each.  Otherwise it is the
 * six integer registers followed by 8 bytes for each floating one.
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
#include "cgdefs.h"
#include "flowgraf.h"
#include "builtin.h"
#include "bind.h"
#include "vargen.h"
#include "simplify.h"   /* MCR_SORT_xxx */
#include "errors.h"

#include "x86ins.h"

#ifdef TARGET_IS_X86_64
#  define IS64 1
#else
#  define IS64 0
#endif
#define WORD X86_PTRSIZE        /* the size of an integer register      */

/* ---------------------------------------------------------------- */
/* Registers                                                          */
/* ---------------------------------------------------------------- */

/* Internal register number to hardware encoding (see target.h).      */
static int const hwreg[NINTREGS] = {
#ifdef TARGET_IS_X86_64
    X86_EDI, X86_ESI, X86_EDX, X86_ECX, X86_R8, X86_R9,
    X86_EAX, X86_R10, X86_R11,
    X86_EBX, X86_R12, X86_R13, X86_R14, X86_R15,
    X86_ESP, X86_EBP
#else
    X86_EAX, X86_EDX, X86_ECX, X86_EBX, X86_ESI, X86_EDI, X86_EBP, X86_ESP,
    -1
#endif
};

#define isfpreg(r) ((r) >= R_F0 && (r) < R_F0+NFLTREGS)
#define xmm(r)     ((int)((r) - R_F0))

/* Hardware registers with 8-bit subregisters: on i386 al, cl, dl, bl; */
/* on x86-64 (with a REX prefix) all of them.                          */
#define byteable(hw) (IS64 || (hw) <= X86_EBX)

static int hw(RealRegister r)
{   if (r < 0 || r >= NINTREGS || hwreg[r] < 0) syserr("x86 hw reg %ld", (long)r);
    return hwreg[r];
}

/* ---------------------------------------------------------------- */
/* The instruction list                                               */
/* ---------------------------------------------------------------- */

X86Ins *x86_insns, *x86_insns_tail;
int32 x86_fnlabel;            /* distinguishes labels between functions */

/* The operand size of the integer operation being translated: 4, or  */
/* on x86-64 8 unless the jopcode is marked J_W32.                     */
static int isz;

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

#ifndef TARGET_IS_X86_64
/* $sym+n */
static X86Op op_symimm(Symstr const *sym, int32 n)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_IMM, o.sym = sym, o.disp = n;
    return o;
}
#endif

/* disp(base,index,scale); on x86-64, with no base or index register  */
/* the address is relative to rip (see asm.c).                        */
static X86Op op_mem(int base, int index, int scale, int32 disp)
{   X86Op o; memclr(&o, sizeof(o));
    o.kind = XO_MEM, o.reg = base, o.index = index, o.scale = scale;
    o.disp = disp;
    return o;
}

/* sym+n in memory. */
static X86Op op_symmem(Symstr const *sym, int32 n)
{   X86Op o = op_mem(-1, -1, 0, n);
    o.sym = sym;
    return o;
}

/* A constant emitted by asm.c. */
static X86Op op_labmem(int32 lab)
{   X86Op o = op_mem(-1, -1, 0, 0);
    o.lab = lab, o.haslab = 1;
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

/* The same, where m is a stem to be given a size suffix.             */
static void insz1(char const *m, int size, X86Op a)
{   ins1(m, a)->size = size;
}

static void insz2(char const *m, int size, X86Op src, X86Op dst)
{   ins2(m, src, dst)->size = size;
}

static void insz3(char const *m, int size, X86Op a, X86Op b, X86Op c)
{   ins3(m, a, b, c)->size = size;
}

static void deflabel(int32 lab)
{   X86Ins *p = newins(NULL);
    p->kind = XI_LABEL, p->op[0] = op_lab(lab);
}

static void directive(char const *text)
{   X86Ins *p = newins(text);
    p->kind = XI_DIRECTIVE;
}

/* Integer register r as an operand of the current size, of size n,   */
/* and as a whole register.                                           */
static X86Op R(RealRegister r)          { return op_reg(hw(r), isz); }
static X86Op Rn(RealRegister r, int n)  { return op_reg(hw(r), n); }
static X86Op RW(RealRegister r)         { return op_reg(hw(r), WORD); }
static X86Op HW(int hwr)                { return op_reg(hwr, WORD); }
static X86Op X(RealRegister r)          { return op_xmm(xmm(r)); }

/* The frame pointer and stack pointer. */
#define FPREG X86_EBP
#define SPREG X86_ESP

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

#ifdef TARGET_IS_X86_64
static int32 const savedregs[] = { 9, 10, 11, 12, 13 };  /* rbx r12-r15 */
#define NSAVEDREGS 5
#else
static int32 const savedregs[] = { 3, 4, 5 };   /* ebx esi edi */
#define NSAVEDREGS 3
#endif

/* Offset from the frame pointer of the scratch slot, and its size.   */
#define SCRATCHSIZE 16
#define scratch_offset() (-WORD*nsaved - SCRATCHSIZE)

#ifdef TARGET_IS_X86_64
static int32 enter_desc;       /* J_ENTER's argument description         */
static int32 homesize;         /* size of the home area                   */

#define HOME_GPRS       (8*NARGREGS)
#define home_offset()   (scratch_offset() - homesize)
#define is_variadic()   ((enter_desc & K_VAFUNC) != 0)

/* The offset from the frame pointer of argument word w.              */
static int32 arg_address(int32 w)
{   int32 nflt = currentfunction.fltargwords;
    if (w < nflt)
        return home_offset() + HOME_GPRS + (is_variadic() ? 16 : 8)*w;
    w -= nflt;
    if (w < NARGREGS) return home_offset() + 8*w;
    return 16 + 8*(w - NARGREGS);
}
#endif

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
#ifdef TARGET_IS_X86_64
        return home_offset() - framepad - q;
#else
        return -4*nsaved - SCRATCHSIZE - framepad - q;
#endif
case BINDADDR_ARG:
#ifdef TARGET_IS_X86_64
        return arg_address(q / 8) + q % 8;
#else
        return 8 + q;
#endif
    }
}

static bool function_saves(int32 r)
{   return member_RealRegSet(&regmaskvec, r);
}

static bool returns_struct_in_memory(void)
{   return (currentfunction.resultrep & MCR_SORT_MASK) == MCR_SORT_STRUCT &&
           currentfunction.nresultregs == 0;
}

static void gen_prologue(void)
{   int32 i;
    insz1("push", WORD, HW(FPREG));
    x86_insns_tail->frame = YES;
    insz2("mov", WORD, HW(SPREG), HW(FPREG));
    x86_insns_tail->frame = YES;
    nsaved = 0;
    for (i = 0; i < NSAVEDREGS; i++)
        if (function_saves(savedregs[i])) {
            insz1("push", WORD, RW(savedregs[i]));
            nsaved++;
        }
#ifdef TARGET_IS_X86_64
    {   int32 nint = k_intregs_(enter_desc), nflt = k_fltregs_(enter_desc);
        homesize = is_variadic() ? HOME_GPRS + 16*NFLTARGREGS :
                   nint + nflt == 0 ? 0 :
                   HOME_GPRS + 8*currentfunction.fltargwords;
        /* On entry rsp = 8 (mod 16), and pushing rbp aligns it.  Choose */
        /* the padding so that rsp is still 16-byte aligned once the     */
        /* saved registers, scratch slot, home area, padding and         */
        /* greatest_stackdepth bytes of locals/outgoing args are pushed. */
        framepad = (-8*nsaved - SCRATCHSIZE - homesize - greatest_stackdepth) & 15;
        {   int32 frame = SCRATCHSIZE + homesize + framepad + greatest_stackdepth;
            if (frame != 0)
            {   insz2("sub", 8, op_imm(frame), HW(SPREG));
                x86_insns_tail->frame = YES;
            }
        }
        /* Give the register arguments their addresses if they need them. */
        if (is_variadic()) nint = NARGREGS, nflt = NFLTARGREGS;
        if (is_variadic() || (procflags & PROC_ARGPUSH)) {
            for (i = 0; i < nint; i++)
                insz2("mov", 8, RW(R_A1+i), op_mem(FPREG, -1, 0, home_offset() + 8*i));
            for (i = 0; i < nflt; i++)
                ins2("movsd", op_xmm(i), op_mem(FPREG, -1, 0,
                     home_offset() + HOME_GPRS + (is_variadic() ? 16 : 8)*i));
        }
        if (returns_struct_in_memory())
            insz2("mov", 8, RW(R_A1), op_mem(FPREG, -1, 0, scratch_offset() + 8));
    }
#else
    /* On entry esp = 12 (mod 16); after pushing ebp and the saved     */
    /* registers it is 8-4*nsaved.  Choose the padding so that esp is  */
    /* 16-byte aligned once the scratch slot, padding and              */
    /* greatest_stackdepth bytes of locals/outgoing args are dropped.  */
    framepad = (8 - 4*nsaved - SCRATCHSIZE - greatest_stackdepth) & 15;
    {   int32 frame = SCRATCHSIZE + framepad + greatest_stackdepth;
        insz2("sub", 4, op_imm(frame), HW(SPREG));
        x86_insns_tail->frame = YES;
    }
#endif
}

#ifdef TARGET_IS_X86_64
/*
 * mip returns a struct in registers in rax and r10 (internal registers 6
 * and 7).  The psABI returns its INTEGER eightbytes in rax then rdx, and
 * its SSE ones in xmm0 then xmm1, as the K_RESULTSSE flags in desc say.
 */
static X86Op struct_result_reg(int32 desc, int32 i)
{   bool sse0 = (desc & K_RESULTSSE0) != 0;
    if (i == 0) return sse0 ? op_xmm(0) : HW(X86_EAX);
    if (desc & K_RESULTSSE1) return op_xmm(sse0 ? 1 : 0);
    return HW(sse0 ? X86_EAX : X86_EDX);
}

/* After a call: from the psABI's registers to rax and r10.           */
static void struct_result_from_abi(int32 desc)
{   int32 n = k_resultregs_(desc);
    if (n == 2) insz2("mov", 8, struct_result_reg(desc, 1), HW(X86_R10));
    if (n >= 1 && (desc & K_RESULTSSE0))
        insz2("mov", 8, struct_result_reg(desc, 0), HW(X86_EAX));
}

/* Before returning: from rax and r10 to the psABI's registers.       */
static void struct_result_to_abi(int32 desc)
{   int32 n = k_resultregs_(desc);
    if (n >= 1 && (desc & K_RESULTSSE0))
        insz2("mov", 8, HW(X86_EAX), struct_result_reg(desc, 0));
    if (n == 2) insz2("mov", 8, HW(X86_R10), struct_result_reg(desc, 1));
}
#endif

static void gen_epilogue(void)
{   int32 i;
#ifdef TARGET_IS_X86_64
    /* The address of a memory struct result is returned in rax.      */
    if (returns_struct_in_memory())
        insz2("mov", 8, op_mem(FPREG, -1, 0, scratch_offset() + 8), HW(X86_EAX));
    struct_result_to_abi(enter_desc);
#else
    int32 rep = currentfunction.resultrep;
    /* The ABI returns floating results in st(0).                     */
    if (rep == MCR_SORT_FLOATING+4) {
        ins2("movss", op_xmm(0), op_mem(FPREG, -1, 0, scratch_offset()));
        ins1("flds", op_mem(FPREG, -1, 0, scratch_offset()));
    } else if (rep == MCR_SORT_FLOATING+8) {
        ins2("movsd", op_xmm(0), op_mem(FPREG, -1, 0, scratch_offset()));
        ins1("fldl", op_mem(FPREG, -1, 0, scratch_offset()));
    } else if (returns_struct_in_memory()) {
        /* ... and the address of a memory struct result in eax.      */
        insz2("mov", 4, op_mem(FPREG, -1, 0, 8), HW(X86_EAX));
    }
#endif
    if (nsaved == 0)
        ins0("leave");          /* mov %rbp, %rsp; pop %rbp           */
    else
    {   insz2("lea", WORD, op_mem(FPREG, -1, 0, -WORD*nsaved), HW(SPREG));
        x86_insns_tail->frame = YES;
        for (i = NSAVEDREGS; --i >= 0; )
            if (function_saves(savedregs[i]))
                insz1("pop", WORD, RW(savedregs[i]));
        insz1("pop", WORD, HW(FPREG));
    }
    x86_insns_tail->frame = YES;
#ifndef TARGET_IS_X86_64
    /* The callee pops the hidden struct-result pointer.              */
    if (returns_struct_in_memory())
        ins1("ret", op_imm(4));
    else
#endif
        ins0("ret");
}

/* A function that makes no calls, saves no registers, and makes no    */
/* other use of the frame pointer (so has nothing in its frame) needs   */
/* no frame: remove the prologue and epilogue instructions that set it  */
/* up and take it down.  The return address is then at the top of the   */
/* stack, which only matters to calls, of which there are none.         */
static void omit_frame(void)
{   X86Ins *p;
    int i;
    if (nsaved != 0) return;
    for (p = x86_insns; p != NULL; p = p->next)
    {   if (p->frame) continue;
        if (p->kind == XI_INSN && p->mnem != NULL && StrEq(p->mnem, "call"))
            return;
        for (i = 0; i < p->nops; i++)
        {   X86Op const *o = &p->op[i];
            if ((o->kind == XO_REG && (o->reg == FPREG || o->reg == SPREG)) ||
                (o->kind == XO_MEM && (o->reg == FPREG || o->reg == SPREG ||
                                       o->index == FPREG)))
                return;
        }
    }
    for (p = x86_insns; p != NULL; p = p->next)
        if (p->frame) p->kind = XI_DELETED;
}

/* ---------------------------------------------------------------- */
/* Peephole optimisation                                              */
/* ---------------------------------------------------------------- */

static X86Ins *next_ins(X86Ins *p)
{   do p = p->next; while (p != NULL && p->kind == XI_DELETED);
    return p;
}

static bool mnem_is(X86Ins const *p, char const *m)
{   return p->kind == XI_INSN && p->mnem != NULL && StrEq(p->mnem, m);
}

static bool is_reg(X86Op const *o, int size)
{   return o->kind == XO_REG && o->size == size;
}

/* Whether the flags are dead after p: whether, on the way from p to   */
/* a return, a call or an instruction that sets all of them, nothing   */
/* reads them.  Only the instructions gen.c makes need be known about, */
/* and any other (or a jump, which isn't followed) is taken to.        */
static bool flags_dead_after(X86Ins *p)
{   static char const *const setters[] = {
        "add", "sub", "and", "or", "xor", "cmp", "test", "neg", "imul",
        "div", "idiv", "ucomiss", "ucomisd", "call", "ret", "leave"
    };
    static char const *const neutral[] = {
        "mov", "movzbl", "movzwl", "movsbl", "movswl", "movslq", "movsd",
        "movss", "movaps", "movd", "lea", "push", "pop", "not", "cltd",
        "cqto", "xorps", "xorpd", "addsd", "addss", "subsd", "subss",
        "mulsd", "mulss", "divsd", "divss", "cvtsi2sd", "cvtsi2ss",
        "cvtss2sd", "cvtsd2ss", "cvttsd2si", "cvttss2si", "rep movsb",
        "rep movsl", "rep movsq", "rep stosb", "rep stosl", "rep stosq"
    };
    unsigned i;
    while ((p = next_ins(p)) != NULL)
    {   if (p->kind == XI_LABEL) continue;
        if (p->kind != XI_INSN || p->mnem == NULL) return NO;
        for (i = 0; i < sizeof(setters)/sizeof(setters[0]); i++)
            if (StrEq(p->mnem, setters[i])) return YES;
        /* A shift by %cl leaves the flags alone if cl is 0.           */
        if ((StrEq(p->mnem, "shl") || StrEq(p->mnem, "shr") ||
             StrEq(p->mnem, "sar")) && p->op[0].kind == XO_IMM)
            return p->op[0].disp != 0;
        for (i = 0; i < sizeof(neutral)/sizeof(neutral[0]); i++)
            if (StrEq(p->mnem, neutral[i])) break;
        if (i == sizeof(neutral)/sizeof(neutral[0])) return NO;
    }
    return NO;
}

static void peephole(void)
{   X86Ins *p, *q;
    for (p = x86_insns; p != NULL; p = next_ins(p))
    {   if (!mnem_is(p, "mov") || p->nops != 2) continue;
        /* mov $0, r -> xor r, r (which is shorter, and breaks any     */
        /* dependency on r) if the flags it sets aren't wanted.         */
        if (p->op[0].kind == XO_IMM && p->op[0].disp == 0 &&
            p->op[0].sym == NULL && p->op[1].kind == XO_REG &&
            (p->size == 4 || p->size == 8) && flags_dead_after(p))
        {   p->mnem = "xor", p->size = 4;
            p->op[1].size = 4, p->op[0] = p->op[1];
            continue;
        }
        /* mov a, b; mov b, a: the second is redundant.  (Not for      */
        /* narrower moves, which zero-extend on x86-64.)                */
        q = next_ins(p);
        if (q != NULL && mnem_is(q, "mov") && q->nops == 2 && !q->frame &&
            p->size == WORD && q->size == WORD &&
            is_reg(&p->op[0], WORD) && is_reg(&p->op[1], WORD) &&
            is_reg(&q->op[0], WORD) && is_reg(&q->op[1], WORD) &&
            q->op[0].reg == p->op[1].reg && q->op[1].reg == p->op[0].reg)
            q->kind = XI_DELETED;
    }
}

/* Align the heads of loops, the targets of backward jumps, to 32      */
/* bytes if that takes no more than 15 bytes of padding, so that how   */
/* fast a small loop runs depends less on where it happens to fall.    */
typedef struct PassedLabel {
    struct PassedLabel *next;
    X86Ins *ins;                /* NULL once aligned                    */
} PassedLabel;

#define NPASSED 64

static void align_loops(void)
{   PassedLabel *passed[NPASSED], *l;
    X86Ins *p;
    memclr(passed, sizeof(passed));
    for (p = x86_insns; p != NULL; p = p->next)
    {   if (p->kind == XI_LABEL)
        {   PassedLabel **h = &passed[p->op[0].disp & (NPASSED-1)];
            l = (PassedLabel *)SynAlloc(sizeof(PassedLabel));
            l->next = *h, l->ins = p, *h = l;
        }
        else if (p->kind == XI_INSN && p->mnem[0] == 'j' && p->nops == 1 &&
                 p->op[0].kind == XO_LAB)
        {   int32 lab = p->op[0].disp;
            for (l = passed[lab & (NPASSED-1)]; l != NULL; l = l->next)
                if (l->ins != NULL && l->ins->op[0].disp == lab)
                {   /* Turn the label into the directive, followed by a  */
                    /* copy of the label.                                */
                    X86Ins *q = (X86Ins *)SynAlloc(sizeof(X86Ins));
                    *q = *l->ins;
                    l->ins->kind = XI_DIRECTIVE;
                    l->ins->mnem = "\t.p2align\t5,,15";
                    l->ins->nops = 0, l->ins->next = q;
                    l->ins = NULL;
                    break;
                }
        }
    }
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
{   if (to != from) insz2("mov", WORD, RW(from), RW(to));
}

static void movx(RealRegister to, RealRegister from)
{   if (to != from) ins2("movaps", X(from), X(to));
}

/* r1 = r2 op r3, op commutative ("add" etc).                         */
static void commutative_rr(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 == r2) insz2(m, isz, R(r3), R(r1));
    else if (r1 == r3) insz2(m, isz, R(r2), R(r1));
    else { movr(r1, r2); insz2(m, isz, R(r3), R(r1)); }
}

/* r1 = r2 op r3, op not commutative.  regalloc ensures r1 != r3      */
/* unless r1 == r2 (TARGET_HAS_2ADDRESS_CODE).                        */
static void asym_rr(char const *m, Icode const *ic)
{   RealRegister r1 = ic->r1.rr, r2 = ic->r2.rr, r3 = ic->r3.rr;
    if (r1 != r2 && r1 == r3) syserr("x86: 2-address clash");
    movr(r1, r2);
    insz2(m, isz, R(r3), R(r1));
}

/* r1 = r2 op k.                                                      */
static void op_rk(char const *m, Icode const *ic)
{   movr(ic->r1.rr, ic->r2.rr);
    insz2(m, isz, op_imm(ic->r3.i), R(ic->r1.rr));
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
        X86Op s = op_mem(FPREG, -1, 0, scratch_offset());
        ins2(m[3] == 's' ? "movss" : "movsd", X(r1), s);
        movx(r1, r3);
        ins2(m, s, X(r1));
    } else { movx(r1, r3); ins2(m, X(r2), X(r1)); }
}

/* The offset of the next constant in the read-only data area.       */
static int32 const_offset(void)
{   return constdata_size();
}

#ifdef TARGET_IS_X86_64
/* Whether sym is defined in this file, and not visible outside it: if  */
/* not, its address is found via the GOT (which the linker can relax    */
/* to a lea), and calls to it go via the PLT, as position-independent   */
/* code needs.                                                          */
static bool is_local_sym(Symstr const *sym)
{   ExtRef *x;
    if (sym == bindsym_(datasegment) || sym == bindsym_(bsssegment) ||
        sym == bindsym_(constdatasegment) || sym == bindsym_(codesegment))
        return YES;
    x = symext_(sym);
    return x != NULL && (x->extflags & xr_defloc);
}
#endif

/* r1 = the address of sym+n.                                         */
static void load_address(RealRegister r1, Symstr const *sym, int32 n)
{
#ifdef TARGET_IS_X86_64
    ExtRef *x = symext_(sym);
    if (x != NULL && (x->extflags & xr_tls))
    {   /* C11's _Thread_local, by the initial-exec model: the offset    */
        /* from the thread pointer (%fs:0) is in the GOT.                */
        X86Op got = op_symmem(sym, 0), tp = op_mem(-1, -1, 0, 0);
        got.reloc = 2;
        tp.seg = 1;
        insz2("mov", 8, got, RW(r1));
        insz2("add", 8, tp, RW(r1));
        if (n != 0) insz2("lea", 8, op_mem(hw(r1), -1, 0, n), RW(r1));
    }
    else if (is_local_sym(sym))
        insz2("lea", 8, op_symmem(sym, n), RW(r1));
    else {
        X86Op got = op_symmem(sym, 0);
        got.reloc = 1;
        insz2("mov", 8, got, RW(r1));
        if (n != 0) insz2("lea", 8, op_mem(hw(r1), -1, 0, n), RW(r1));
    }
#else
    insz2("mov", 4, op_symimm(sym, n), RW(r1));
#endif
}

/* A floating constant: put it in the read-only data area.            */
static void load_fpconst(RealRegister r1, FloatCon *fc, int32 len)
{   int32 off;
    DataAreaSort old = SetDataArea(DS_Const);
    padstatic(len);
    off = const_offset();
    gendcE(len, fc);
    SetDataArea(old);
    ins2(len == 4 ? "movss" : "movsd",
         op_symmem(bindsym_(constdatasegment), off), X(r1));
}

/* Constants emitted by asm.c if used.                                */
bool x86_negmask_used;          /* sign-bit masks for negating floats  */
bool x86_two32_used;            /* 2^32 as a double                    */
bool x86_two63_used;            /* 2^63 as a double                    */

#ifdef TARGET_IS_X86_64
/* Scratch registers that RealRegisterUse (mcdep.c) keeps free for the */
/* unsigned conversions.                                              */
#define SCRATCH_INT     X86_R11
#define SCRATCH_XMM     15
#endif

/* ---------------------------------------------------------------- */
/* Switch tables                                                      */
/* ---------------------------------------------------------------- */

static int32 casetab_label, casedef_label, casetab_entries;
static LabelNumber *casetab_default;

/* On x86-64, entries are offsets from the table, so that the code is  */
/* position independent.                                                */
static void casetab_entry(int32 lab)
{   X86Ins *p = IS64 ? ins2(".long", op_lab(lab), op_lab(casetab_label)) :
                       ins1(".long", op_lab(lab));
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
/* Block moves                                                        */
/* ---------------------------------------------------------------- */

/* rep movs or rep stos of n bytes, in the largest units that fit.    */
static void rep_string(char const *m, int32 n)
{   int size = IS64 && (n & 7) == 0 ? 8 : (n & 3) == 0 ? 4 : 1;
    static char const *const movs[] = { 0, "rep movsb", 0, 0, "rep movsl",
                                        0, 0, 0, "rep movsq" };
    static char const *const stos[] = { 0, "rep stosb", 0, 0, "rep stosl",
                                        0, 0, 0, "rep stosq" };
    insz2("mov", 4, op_imm(n / size), op_reg(X86_ECX, 4));
    ins0((m[0] == 'm' ? movs : stos)[size]);
}

/* ---------------------------------------------------------------- */
/* Calls                                                              */
/* ---------------------------------------------------------------- */

static void gen_call(Icode const *ic)
{   J_OPCODE opm = ic->op & J_TABLE_BITS;
    int32 desc = ic->r2.i;
#ifdef TARGET_IS_X86_64
    X86Op target;
    if (opm == J_CALLR) target = RW(ic->r3.rr);
    if (desc & K_VACALL) {
        /* al gives the number of vector registers used.              */
        if (opm == J_CALLR && hw(ic->r3.rr) == X86_EAX) {
            target = HW(SCRATCH_INT);
            insz2("mov", 8, HW(X86_EAX), target);
        }
        insz2("mov", 4, op_imm(k_fltregs_(desc)), op_reg(X86_EAX, 4));
    }
    if (opm == J_CALLK) {
        X86Op f = op_sym(ic->r3.sym);
        obj_symref(ic->r3.sym, xr_code, 0);
        f.reloc = !is_local_sym(ic->r3.sym);
        ins1("call", f);
    } else
        ins1("call", op_star(target));
    struct_result_from_abi(desc);
#else
    if (opm == J_CALLK) {
        obj_symref(ic->r3.sym, xr_code, 0);
        ins1("call", op_sym(ic->r3.sym));
    } else
        ins1("call", op_star(RW(ic->r3.rr)));
    if (desc & (K_FLTRESULT|K_DBLRESULT)) {
        /* Move the result from st(0) to xmm0.                        */
        X86Op s = op_mem(FPREG, -1, 0, scratch_offset());
        bool d = (desc & K_DBLRESULT) != 0;
        ins1(d ? "fstpl" : "fstps", s);
        ins2(d ? "movsd" : "movss", s, op_xmm(0));
    }
    /* A memory struct result's pointer is popped by callee.          */
    if (desc & K_STRUCTRESULT)
        insz2("sub", 4, op_imm(4), HW(SPREG));
#endif
}

/* ---------------------------------------------------------------- */
/* Conversions between integer and floating point                     */
/* ---------------------------------------------------------------- */

/* r1 (float or double) = r3 (integer).                               */
static void gen_float(J_OPCODE op, RealRegister r1, RealRegister r3)
{   bool dbl = (op & J_TABLE_BITS) == J_FLTDR;
#ifdef TARGET_IS_X86_64
    char const *cvt = dbl ? "cvtsi2sd" : "cvtsi2ss";
    if (!(op & J_UNSIGNED))
        insz2(cvt, isz, R(r3), X(r1));
    else if (isz == 4) {
        /* Zero extend, and convert as a (positive) 64-bit number.    */
        insz2("mov", 4, Rn(r3, 4), op_reg(SCRATCH_INT, 4));
        insz2(cvt, 8, HW(SCRATCH_INT), X(r1));
    } else {
        /* If the top bit is set, halve the number (keeping the bottom  */
        /* bit, so as to round correctly), convert, and double.         */
        int32 big = newlabel(), done = newlabel(), odd = newlabel();
        insz2("test", 8, RW(r3), RW(r3));
        ins1("js", op_lab(big));
        insz2(cvt, 8, RW(r3), X(r1));
        ins1("jmp", op_lab(done));
        deflabel(big);
        insz2("mov", 8, RW(r3), HW(SCRATCH_INT));
        insz1("shr", 8, HW(SCRATCH_INT));
        ins1("jnc", op_lab(odd));
        insz2("or", 8, op_imm(1), HW(SCRATCH_INT));
        deflabel(odd);
        insz2(cvt, 8, HW(SCRATCH_INT), X(r1));
        ins2(dbl ? "addsd" : "addss", X(r1), X(r1));
        deflabel(done);
    }
#else
    if (op & J_UNSIGNED) {
        /* Convert as signed, then add 2^32 if the top bit was set; */
        /* for float, do that in double to avoid double rounding.   */
        int32 skip = newlabel();
        ins2("cvtsi2sd", R(r3), X(r1));
        insz2("test", 4, R(r3), R(r3));
        ins1("jns", op_lab(skip));
        x86_two32_used = YES;
        ins2("addsd", op_labmem(XLAB_TWO32), X(r1));
        deflabel(skip);
        if (!dbl) ins2("cvtsd2ss", X(r1), X(r1));
    } else
        ins2(dbl ? "cvtsi2sd" : "cvtsi2ss", R(r3), X(r1));
#endif
}

/* r1 (integer) = r3 (float or double), truncating.                   */
static void gen_fix(J_OPCODE op, RealRegister r1, RealRegister r3)
{   bool dbl = (op & J_TABLE_BITS) == J_FIXDR;
#ifdef TARGET_IS_X86_64
    char const *cvt = dbl ? "cvttsd2si" : "cvttss2si";
    if (!(op & J_UNSIGNED))
        insz2(cvt, isz, X(r3), R(r1));
    else if (isz == 4)
        /* The result fits in a signed 64-bit number.                 */
        insz2(cvt, 8, X(r3), RW(r1));
    else {
        /* Numbers of 2^63 or more have 2^63 subtracted before the     */
        /* conversion, and added (by flipping the top bit) after.      */
        int32 big = newlabel(), done = newlabel();
        X86Op s = op_xmm(SCRATCH_XMM);
        x86_two63_used = YES;
        if (dbl) ins2("movaps", X(r3), s);
        else ins2("cvtss2sd", X(r3), s);
        ins2("ucomisd", op_labmem(XLAB_TWO63), s);
        ins1("jae", op_lab(big));
        insz2("cvttsd2si", 8, s, RW(r1));
        ins1("jmp", op_lab(done));
        deflabel(big);
        ins2("subsd", op_labmem(XLAB_TWO63), s);
        insz2("cvttsd2si", 8, s, RW(r1));
        insz2("btc", 8, op_imm(63), RW(r1));
        deflabel(done);
    }
#else
    if (op & J_UNSIGNED) {
        /* Use the x87 to convert to a 64-bit integer, truncating,  */
        /* and take the low word.                                   */
        int32 s = scratch_offset();
        X86Op cw_old = op_mem(FPREG, -1, 0, s + 8);
        X86Op cw_new = op_mem(FPREG, -1, 0, s + 10);
        X86Op val = op_mem(FPREG, -1, 0, s);
        ins2(dbl ? "movsd" : "movss", X(r3), val);
        ins1(dbl ? "fldl" : "flds", val);
        ins1("fnstcw", cw_old);
        ins2("movzwl", cw_old, R(r1));
        insz2("or", 4, op_imm(0x0c00), R(r1));      /* round to zero    */
        insz2("mov", 2, Rn(r1, 2), cw_new);
        ins1("fldcw", cw_new);
        ins1("fistpll", val);
        ins1("fldcw", cw_old);
        insz2("mov", 4, val, R(r1));
    } else
        ins2(dbl ? "cvttsd2si" : "cvttss2si", X(r3), R(r1));
#endif
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

    isz = IS64 && !(op & J_W32) ? 8 : 4;

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
#ifdef TARGET_IS_X86_64
        enter_desc = m;
#endif
        gen_prologue();
        return;

    case J_ENDPROC:
        if (retlab_used) {
            deflabel(LAB_RET);
            gen_epilogue();
        }
        omit_frame();
        peephole();
        align_loops();
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
        /* On x86-64, cg.c makes the index a 64-bit value.            */
        casetab_label = newlabel();
        casedef_label = newlabel();
        casetab_entries = m;
        casetab_default = NULL;
        insz2("cmp", WORD, op_imm(m-1), RW(r1));
        ins1("jae", op_lab(casedef_label));
#ifdef TARGET_IS_X86_64
        /* RealRegisterUse keeps r1 out of r10 and r11.                */
        insz2("lea", 8, op_labmem(casetab_label), HW(X86_R11));
        ins2("movslq", op_mem(X86_R11, hw(r1), 4, 4), HW(X86_R10));
        insz2("add", 8, HW(X86_R11), HW(X86_R10));
        ins1("jmp", op_star(HW(X86_R10)));
#else
        {   X86Op t = op_mem(-1, hw(r1), 4, 4);
            t.lab = casetab_label, t.haslab = 1;
            ins1("jmp", op_star(t));
        }
#endif
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
#ifdef TARGET_IS_X86_64
    case J_MOVLR:                       /* from J_LDRLV of a register  */
#endif
        movr(r1, r3);
        return;
    case J_MOVK:
        /* On x86-64, a 32-bit move zero extends: use it if that gives */
        /* the right value.                                           */
        insz2("mov", m >= 0 ? 4 : WORD, op_imm(m), Rn(r1, m >= 0 ? 4 : WORD));
        return;
    case J_ADCON:
        load_address(r1, ic->r3.sym, ic->r2.i);
        return;
    case J_ADCONLL:
        /* The address of a long long literal.                        */
        {   int32 off;
            DataAreaSort old = SetDataArea(DS_Const);
            padstatic(8);
            off = const_offset();
            gendcI(4, (int32)ic->r3.i64->bin.i.lo);
            gendcI(4, (int32)ic->r3.i64->bin.i.hi);
            SetDataArea(old);
            load_address(r1, bindsym_(constdatasegment), off);
        }
        return;
    case J_STRING:
        {   int32 off;
            DataAreaSort old = SetDataArea(DS_Const);
            off = const_offset();
            vg_genstring(ic->r3.s, stringlength(ic->r3.s)+1, 0);
            padstatic(4);
            SetDataArea(old);
            load_address(r1, bindsym_(constdatasegment), off);
        }
        return;

    /* ---- integer loads and stores ---- */
    case J_LDRK: case J_LDRR:
        insz2("mov", 4, memop(ic, opm == J_LDRR), Rn(r1, 4));
        return;
    case J_LDRBK: case J_LDRBR:
        ins2(op & J_SIGNED ? "movsbl" : "movzbl", memop(ic, opm == J_LDRBR), Rn(r1, 4));
        return;
    case J_LDRWK: case J_LDRWR:
        ins2(op & J_SIGNED ? "movswl" : "movzwl", memop(ic, opm == J_LDRWR), Rn(r1, 4));
        return;
    case J_STRK: case J_STRR:
        insz2("mov", 4, Rn(r1, 4), memop(ic, opm == J_STRR));
        return;
    case J_STRWK: case J_STRWR:
        insz2("mov", 2, Rn(r1, 2), memop(ic, opm == J_STRWR));
        return;
    case J_STRBK: case J_STRBR:
        {   X86Op mem = memop(ic, opm == J_STRBR);
            if (byteable(hw(r1)))
                insz2("mov", 1, Rn(r1, 1), mem);
            else {
                /* esi/edi have no byte form: borrow a byteable       */
                /* register not used in the address.                  */
                int t;
                for (t = X86_EAX; t <= X86_EBX; t++)
                    if (t != mem.reg && t != mem.index) break;
                insz1("push", 4, op_reg(t, 4));
                insz2("mov", 4, Rn(r1, 4), op_reg(t, 4));
                insz2("mov", 1, op_reg(t, 1), mem);
                insz1("pop", 4, op_reg(t, 4));
            }
        }
        return;
#ifdef TARGET_IS_X86_64
    case J_LDRLK: case J_LDRLR:
        insz2("mov", 8, memop(ic, opm == J_LDRLR), RW(r1));
        return;
    case J_STRLK: case J_STRLR:
        insz2("mov", 8, RW(r1), memop(ic, opm == J_STRLR));
        return;
#endif

    /* ---- integer arithmetic ---- */
    case J_ADDK:
        if (r1 == r2) insz2("add", isz, op_imm(m), R(r1));
        else insz2("lea", isz, op_mem(hw(r2), -1, 0, m), R(r1));
        return;
    case J_ADDR:
        if (r1 != r2 && r1 != r3)
            insz2("lea", isz, op_mem(hw(r2), hw(r3), 1, 0), R(r1));
        else commutative_rr("add", ic);
        return;
    case J_SUBK:
        if (r1 == r2) insz2("sub", isz, op_imm(m), R(r1));
        else insz2("lea", isz, op_mem(hw(r2), -1, 0, -m), R(r1));
        return;
    case J_SUBR:
        asym_rr("sub", ic);
        return;
    case J_RSBK:                        /* r1 = k - r2 */
        if (r1 == r2) {
            insz1("neg", isz, R(r1));
            insz2("add", isz, op_imm(m), R(r1));
        } else {
            insz2("mov", isz, op_imm(m), R(r1));
            insz2("sub", isz, R(r2), R(r1));
        }
        return;
    case J_RSBR:                        /* r1 = r3 - r2 */
        if (r1 == r3) insz2("sub", isz, R(r2), R(r1));
        else if (r1 == r2) {
            insz1("neg", isz, R(r1));
            insz2("add", isz, R(r3), R(r1));
        } else {
            movr(r1, r3);
            insz2("sub", isz, R(r2), R(r1));
        }
        return;
    case J_ANDK: op_rk("and", ic); return;
    case J_ORRK: op_rk("or", ic);  return;
    case J_EORK: op_rk("xor", ic); return;
    case J_ANDR: commutative_rr("and", ic); return;
    case J_ORRR: commutative_rr("or", ic);  return;
    case J_EORR: commutative_rr("xor", ic); return;
    case J_MULK:
        if (m == 1 && isz == WORD)
            movr(r1, r2);
        else if (m == 1)                /* (zero-extending)            */
            insz2("mov", isz, R(r2), R(r1));
        else
            insz3("imul", isz, op_imm(m), R(r2), R(r1));
        return;
    case J_MULR:
        commutative_rr("imul", ic);
        return;
    case J_DIVR: case J_REMR:
        /* RealRegisterUse keeps r1, r2 and r3 out of eax and edx.    */
        insz2("mov", isz, R(r2), op_reg(X86_EAX, isz));
        if (op & J_UNSIGNED) {
            insz2("xor", 4, op_reg(X86_EDX, 4), op_reg(X86_EDX, 4));
            insz1("div", isz, R(r3));
        } else {
            ins0(isz == 8 ? "cqto" : "cltd");
            insz1("idiv", isz, R(r3));
        }
        insz2("mov", isz, op_reg(opm == J_DIVR ? X86_EAX : X86_EDX, isz), R(r1));
        return;
    case J_SHLK:
        op_rk("shl", ic);
        return;
    case J_SHRK:
        op_rk(op & J_UNSIGNED ? "shr" : "sar", ic);
        return;
    case J_SHLR: case J_SHRR:
        /* RealRegisterUse keeps r1 and r2 out of ecx.                */
        insz2("mov", 4, Rn(r3, 4), op_reg(X86_ECX, 4));
        movr(r1, r2);
        insz2(opm == J_SHLR ? "shl" : op & J_UNSIGNED ? "shr" : "sar", isz,
              op_reg(X86_ECX, 1), R(r1));
        return;
    case J_NEGR:
        movr(r1, r3);
        insz1("neg", isz, R(r1));
        return;
    case J_NOTR:
        movr(r1, r3);
        insz1("not", isz, R(r1));
        return;
    case J_EXTEND:
        /* r3: 0 or 1 = from byte, 2 = from halfword, and on x86-64,  */
        /* 3 = from word, and 4 = from word, zero extending.          */
        if (m == 4) insz2("mov", 4, Rn(r2, 4), Rn(r1, 4));
        else if (m == 3) ins2("movslq", Rn(r2, 4), RW(r1));
        else if (m == 2) insz2("movsw", WORD, Rn(r2, 2), RW(r1));
        else if (byteable(hw(r2))) insz2("movsb", WORD, Rn(r2, 1), RW(r1));
        else {
            movr(r1, r2);
            insz2("shl", 4, op_imm(24), R(r1));
            insz2("sar", 4, op_imm(24), R(r1));
        }
        return;

    /* ---- compares ---- */
    case J_CMPK:
        if (m == 0) insz2("test", isz, R(r2), R(r2));
        else insz2("cmp", isz, op_imm(m), R(r2));
        cmp_is_fp = NO;
        return;
    case J_CMPR:
        insz2("cmp", isz, R(r3), R(r2));
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
        gen_call(ic);
        return;

    /* ---- block moves (r1 = dest, r2 = source, r3 = byte count) ---- */
    case J_MOVC:
#ifdef TARGET_IS_X86_64
        if (m <= SMALL_BLOCK)
        {   /* RealRegisterUse keeps r1 and r2 out of r11.             */
            int32 off = 0, k;
            for (k = 8; k >= 1; k >>= 1)
                for (; m - off >= k; off += k)
                {   insz2("mov", k, op_mem(hw(r2), -1, 0, off), op_reg(SCRATCH_INT, k));
                    insz2("mov", k, op_reg(SCRATCH_INT, k), op_mem(hw(r1), -1, 0, off));
                }
            return;
        }
#endif
        insz1("push", WORD, HW(X86_ESI));
        insz1("push", WORD, HW(X86_EDI));
        insz1("push", WORD, HW(X86_ECX));
        insz1("push", WORD, RW(r2));
        insz1("push", WORD, RW(r1));
        insz1("pop", WORD, HW(X86_EDI));
        insz1("pop", WORD, HW(X86_ESI));
        rep_string("movs", m);
        insz1("pop", WORD, HW(X86_ECX));
        insz1("pop", WORD, HW(X86_EDI));
        insz1("pop", WORD, HW(X86_ESI));
        return;
    case J_CLRC:
#ifdef TARGET_IS_X86_64
        if (m <= SMALL_BLOCK)
        {   int32 off = 0, k;
            for (k = 8; k >= 1; k >>= 1)
                for (; m - off >= k; off += k)
                    insz2("mov", k, op_imm(0), op_mem(hw(r1), -1, 0, off));
            return;
        }
#endif
        insz1("push", WORD, HW(X86_EDI));
        insz1("push", WORD, HW(X86_ECX));
        insz1("push", WORD, HW(X86_EAX));
        insz1("push", WORD, RW(r1));
        insz1("pop", WORD, HW(X86_EDI));
        insz2("xor", 4, op_reg(X86_EAX, 4), op_reg(X86_EAX, 4));
        rep_string("stos", m);
        insz1("pop", WORD, HW(X86_EAX));
        insz1("pop", WORD, HW(X86_ECX));
        insz1("pop", WORD, HW(X86_EDI));
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
        x86_negmask_used = YES;
        movx(r1, r3);
        ins2(opm == J_NEGFR ? "xorps" : "xorpd",
             op_labmem(opm == J_NEGFR ? XLAB_NEGMASKF : XLAB_NEGMASKD), X(r1));
        return;
    case J_FLTFR: case J_FLTDR:
        gen_float(op, r1, r3);
        return;
    case J_FIXFR: case J_FIXDR:
        gen_fix(op, r1, r3);
        return;
    case J_MOVIDR:                      /* r2 = low word, r3 = high word */
        {   int32 s = scratch_offset();
            insz2("mov", 4, Rn(r2, 4), op_mem(FPREG, -1, 0, s));
            insz2("mov", 4, Rn(r3, 4), op_mem(FPREG, -1, 0, s + 4));
            ins2("movsd", op_mem(FPREG, -1, 0, s), X(r1));
        }
        return;
    case J_MOVDIR:                      /* r1 = low word, r2 = high word */
        {   int32 s = scratch_offset();
            ins2("movsd", X(r3), op_mem(FPREG, -1, 0, s));
            insz2("mov", 4, op_mem(FPREG, -1, 0, s), Rn(r1, 4));
            insz2("mov", 4, op_mem(FPREG, -1, 0, s + 4), Rn(r2, 4));
        }
        return;
    case J_MOVFDR:                      /* float -> double */
        ins2("cvtss2sd", X(r3), X(r1));
        return;
    case J_MOVDFR:                      /* double -> float */
        ins2("cvtsd2ss", X(r3), X(r1));
        return;
    case J_MOVIFR:                      /* int bits -> float reg */
        ins2("movd", Rn(r3, 4), X(r1));
        return;
    case J_MOVFIR:                      /* float bits -> int reg */
        ins2("movd", X(r3), Rn(r1, 4));
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
#ifdef TARGET_IS_X86_64
    enter_desc = homesize = 0;
#endif
    x86_fnlabel++;
}

void localcg_tidy(void)
{
}

void mcdep_init(void)
{   x86_negmask_used = NO;
    x86_two32_used = NO;
    x86_two63_used = NO;
}

void setlabel(LabelNumber *l)
{   IGNORE(l);
}

void branch_round_literals(LabelNumber *m)
{   IGNORE(m);
}

/* end of x86/gen.c */
