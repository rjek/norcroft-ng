/*
 * x86/asm.c -- GNU assembler (AT&T syntax) output for the x86 back end.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Also keeps the symbol table (obj_symref), since there is no object
 * file writer yet: the assembler turns this output into ELF.
 */

#include <string.h>
#include <ctype.h>

#include "globals.h"
#include "mcdep.h"
#include "mcdpriv.h"
#include "xrefs.h"
#include "store.h"
#include "codebuf.h"
#include "builtin.h"
#include "version.h"
#include "errors.h"

#include "x86ins.h"

FILE *asmstream;
FILE *objstream;
ExtRef *obj_symlist;
CodeXref *codexrefs;

/* ---------------------------------------------------------------- */
/* Symbols                                                            */
/* ---------------------------------------------------------------- */

int32 obj_symref(Symstr *s, int flags, int32 loc)
{   ExtRef *x = symext_(s);
    if (x == NULL) {
        x = (ExtRef *)GlobAlloc(SU_Xsym, sizeof(ExtRef));
        memclr(x, sizeof(ExtRef));
        x->extcdr = obj_symlist;
        x->extsym = s;
        reg_setallused(&x->usedregs);
        obj_symlist = symext_(s) = x;
    }
    /* A definition overrides the code/data-ness of earlier references */
    /* (and vice versa), as in the other object formatters.            */
    if (flags & (xr_defloc+xr_defext)) {
        if (!(x->extflags & (xr_defloc+xr_defext)))
            x->extflags &= ~(xr_code+xr_data);
    } else if (x->extflags & (xr_defloc+xr_defext))
        flags &= ~(xr_code+xr_data);
    x->extflags |= flags;
    if (flags & (xr_defloc+xr_defext))
        x->extoffset = loc;
    else if (loc > 0 && !(flags & xr_code) && loc > x->extoffset)
        x->extoffset = loc;             /* size of a common block       */
    return -1;
}

int32 obj_symdef(Symstr *s, int flags, int32 loc)
{   symext_(s) = NULL;
    return obj_symref(s, flags, loc);
}

void obj_init(void)
{   obj_symlist = NULL;
    codexrefs = NULL;
}

void obj_header(void) {}
void obj_trailer(void) {}
void obj_common_start(Symstr *name) { IGNORE(name); }
void obj_common_end(void) {}

/* ---------------------------------------------------------------- */
/* Names                                                              */
/* ---------------------------------------------------------------- */

static void pr_sym(Symstr const *s)
{   FILE *as = asmstream;
    char const *name = symname_(s);
    char const *p;
    if (s == bindsym_(datasegment))      { fputs(".Ldata", as); return; }
    if (s == bindsym_(bsssegment))       { fputs(".Lbss", as); return; }
    if (s == bindsym_(constdatasegment)) { fputs(".Lconst", as); return; }
    for (p = name; *p != 0; p++)
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '.') break;
    if (*p == 0) fputs(name, as);
    else fprintf(as, "\"%s\"", name);
}

static void pr_lab(int32 lab)
{   if (lab == XLAB_NEGMASKF) fputs(".Lnegmaskf", asmstream);
    else if (lab == XLAB_NEGMASKD) fputs(".Lnegmaskd", asmstream);
    else if (lab == XLAB_TWO32) fputs(".Ltwo32", asmstream);
    else if (lab == XLAB_TWO63) fputs(".Ltwo63", asmstream);
    else if (lab < 0) fprintf(asmstream, ".L%ld_x%ld", (long)x86_fnlabel, (long)-lab);
    else fprintf(asmstream, ".L%ld_%ld", (long)x86_fnlabel, (long)lab);
}

/* ---------------------------------------------------------------- */
/* Instructions                                                       */
/* ---------------------------------------------------------------- */

static char const *const reg64[] =
    { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
      "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
static char const *const reg32[] =
    { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
      "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
static char const *const reg16[] =
    { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
      "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
static char const *const reg8[] =
    { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
      "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };

/* The name of a register used in an address.                         */
#define addrreg(r) (X86_PTRSIZE == 8 ? reg64[r] : reg32[r])

static void pr_disp(X86Op const *o)
{   FILE *as = asmstream;
    bool any = NO;
    if (o->sym != NULL) {
        pr_sym(o->sym);
        if (o->reloc) fputs("@GOTPCREL", as);
        any = YES;
    }
    if (o->haslab) {
        if (any) fputc('+', as);
        pr_lab(o->lab);
        any = YES;
    }
    if (o->disp != 0 || !any) {
        if (any && o->disp >= 0) fputc('+', as);
        fprintf(as, "%ld", (long)o->disp);
    }
}

static void pr_op(X86Op const *o)
{   FILE *as = asmstream;
    if (o->star) fputc('*', as);
    switch (o->kind)
    {
    case XO_REG:
        fprintf(as, "%%%s", o->size == 1 ? reg8[o->reg] :
                            o->size == 2 ? reg16[o->reg] :
                            o->size == 8 ? reg64[o->reg] : reg32[o->reg]);
        break;
    case XO_XMM:
        fprintf(as, "%%xmm%d", o->reg);
        break;
    case XO_IMM:
        fputc('$', as);
        pr_disp(o);
        break;
    case XO_MEM:
        if (o->sym != NULL || o->haslab || o->disp != 0 || o->reg < 0)
            pr_disp(o);
        if (o->reg >= 0 || o->index >= 0) {
            fputc('(', as);
            if (o->reg >= 0) fprintf(as, "%%%s", addrreg(o->reg));
            if (o->index >= 0)
                fprintf(as, ",%%%s,%d", addrreg(o->index), o->scale);
            fputc(')', as);
        } else if (X86_PTRSIZE == 8)
            /* An absolute address would need a 32-bit relocation.    */
            fputs("(%rip)", as);
        break;
    case XO_LAB:
        pr_lab(o->disp);
        break;
    case XO_SYM:
        pr_sym(o->sym);
        if (o->reloc) fputs("@PLT", as);
        break;
    default:
        syserr("x86 asm: operand kind %d", o->kind);
    }
}

static void pr_ins(X86Ins const *p)
{   FILE *as = asmstream;
    int i;
    switch (p->kind)
    {
    case XI_LABEL:
        pr_lab(p->op[0].disp);
        fputs(":\n", as);
        return;
    case XI_DIRECTIVE:
        fprintf(as, "%s\n", p->mnem);
        return;
    case XI_DATA:
        if (p->nops == 2) {
            fprintf(as, "\t%s\t", p->mnem);
            pr_op(&p->op[0]);
            fputc('-', as);
            pr_op(&p->op[1]);
            fputc('\n', as);
            return;
        }
        break;
    }
    fprintf(as, "\t%s", p->mnem);
    if (p->size != 0)
        fputc(p->size == 1 ? 'b' : p->size == 2 ? 'w' : p->size == 4 ? 'l' : 'q', as);
    for (i = 0; i < p->nops; i++) {
        fputs(i == 0 ? "\t" : ", ", as);
        pr_op(&p->op[i]);
    }
    fputc('\n', as);
}

static bool is_global(Symstr const *s)
{   ExtRef *x = symext_(s);
    return x != NULL && (x->extflags & xr_defext);
}

void display_assembly_code(Symstr const *name)
{   FILE *as = asmstream;
    X86Ins const *p;
    /* The code segment's own symbol has no code of its own.          */
    if (name == NULL || name == bindsym_(codesegment)) return;
    fputs("\n\t.text\n\t.p2align\t4\n", as);
    if (is_global(name)) {
        fputs("\t.globl\t", as); pr_sym(name); fputc('\n', as);
    }
    fputs("\t.type\t", as); pr_sym(name); fputs(", @function\n", as);
    pr_sym(name); fputs(":\n", as);
    for (p = x86_insns; p != NULL; p = p->next) pr_ins(p);
    fputs("\t.size\t", as); pr_sym(name); fputs(", .-", as);
    pr_sym(name); fputc('\n', as);
}

/* ---------------------------------------------------------------- */
/* Data                                                               */
/* ---------------------------------------------------------------- */

static void asm_data(DataInit *p)
{ FILE *as = asmstream;
  bool adcon_hi = NO;       /* the next word is the top of an address  */
  for (; p != 0; p = p->datacdr)
  { int32 sort = p->sort;
    IPtr rpt = p->rpt, len = p->len;
    union { unsigned32 l;
            unsigned16 w[2];
            unsigned8 b[4];
          } val;
    val.l = (unsigned32)p->val;
    if (adcon_hi) {
        /* mip gives an 8-byte address as a 4-byte LIT_ADCON followed   */
        /* by 4 zero bytes (see gendcAX()), which the .quad covers.     */
        adcon_hi = NO;
        if (sort != LIT_NUMBER || len != 4 || p->val != 0)
            syserr("x86 asm: address not followed by zero word");
        if (--rpt == 0) continue;
    }
    switch (sort)
    {   case LIT_LABEL:
        {   Symstr *s = (Symstr *)rpt;
            /* The area's own label is emitted by asm_trailer().        */
            if (s == bindsym_(datasegment) ||
                s == bindsym_(constdatasegment)) break;
            if (is_global(s)) {
                fputs("\t.globl\t", as); pr_sym(s); fputc('\n', as);
            }
            fputs("\t.type\t", as); pr_sym(s); fputs(", @object\n", as);
            pr_sym(s);
            fputs(":\n", as);
            break;
        }
        default:
            syserr(syserr_asm_trailer, (long)sort);
        case LIT_BBBB:
            fprintf(as, "\t.byte\t%u, %u, %u, %u\n",
                    val.b[0], val.b[1], val.b[2], val.b[3]);
            break;
        case LIT_BBBX:
            fprintf(as, "\t.byte\t%u, %u, %u\n", val.b[0], val.b[1], val.b[2]);
            break;
        case LIT_BBX: case LIT_BB:
            fprintf(as, "\t.byte\t%u, %u\n", val.b[0], val.b[1]);
            break;
        case LIT_BXXX:
            fprintf(as, "\t.byte\t%u\n", val.b[0]);
            break;
        case LIT_HH:
            fprintf(as, "\t.short\t%u, %u\n", val.w[0], val.w[1]);
            break;
        case LIT_HX: case LIT_H:
            fprintf(as, "\t.short\t%u\n", val.w[0]);
            break;
        case LIT_BBH:
            fprintf(as, "\t.byte\t%u, %u\n\t.short\t%u\n",
                    val.b[0], val.b[1], val.w[1]);
            break;
        case LIT_HBX:
            fprintf(as, "\t.short\t%u\n\t.byte\t%u\n", val.w[0], val.b[2]);
            break;
        case LIT_HBB:
            fprintf(as, "\t.short\t%u\n\t.byte\t%u, %u\n",
                    val.w[0], val.b[2], val.b[3]);
            break;
        case LIT_NUMBER:
            if (len != 4) syserr(syserr_asm_data, (long)len);
            if (rpt == 1)
                fprintf(as, "\t.long\t0x%.8lx\n", (unsigned long)val.l);
            else
                fprintf(as, "\t.zero\t%ld\n", (long)(rpt*len));
            break;
        case LIT_FPNUM:
        {   FloatCon *f = (FloatCon *)p->val;
            if (len == 4)
                fprintf(as, "\t.long\t0x%.8lx\n", (unsigned long)f->floatbin.fb.val);
            else
                fprintf(as, "\t.long\t0x%.8lx, 0x%.8lx\n",
                        (unsigned long)f->floatbin.db.lsd,
                        (unsigned long)f->floatbin.db.msd);
            break;
        }
        case LIT_ADCON:              /* (possibly external) name + offset */
            if (X86_PTRSIZE == 8) {
                if (rpt != 1) syserr("x86 asm: repeated address");
                adcon_hi = YES;
            }
            while (rpt--) {
                fputs(X86_PTRSIZE == 8 ? "\t.quad\t" : "\t.long\t", as);
                pr_sym((Symstr *)len);
                if (val.l != 0) fprintf(as, "+%ld", (long)(int32)val.l);
                fputc('\n', as);
            }
            break;
    }
  }
}

typedef struct ExtRefList {
    struct ExtRefList *cdr;
    ExtRef *car;
} ExtRefList;

static void asm_bss(void)
{   FILE *as = asmstream;
    int32 n = 0;
    ExtRef *x;
    ExtRefList *syms = NULL;
    fputs("\n\t.bss\n\t.p2align\t4\n.Lbss:\n", as);
    /* Sort the BSS symbols by offset.                                */
    for (x = obj_symlist; x != NULL; x = x->extcdr)
        if (x->extflags & xr_bss) {
            ExtRefList **prev = &syms, *p;
            for (; (p = *prev) != NULL; prev = &cdr_(p))
                if (x->extoffset < car_(p)->extoffset) break;
            *prev = (ExtRefList *)syn_cons2(*prev, x);
        }
    for (; syms != NULL; syms = cdr_(syms)) {
        x = car_(syms);
        if (x->extoffset != n) fprintf(as, "\t.zero\t%ld\n", (long)(x->extoffset - n));
        n = x->extoffset;
        if (x->extflags & xr_defext) {
            fputs("\t.globl\t", as); pr_sym(x->extsym); fputc('\n', as);
        }
        pr_sym(x->extsym);
        fputs(":\n", as);
    }
    if (n != bss_size) fprintf(as, "\t.zero\t%ld\n", (long)(bss_size - n));
}

void asm_header(void)
{   fprintf(asmstream, "# generated by %s\n", CC_BANNER);
}

void asm_trailer(void)
{   FILE *as = asmstream;
    ExtRef *x;
    if (constdata_size() != 0) {
        /* Constant data containing addresses must be writable by the  */
        /* dynamic linker in position-independent (x86-64) code.       */
        DataInit *d;
        char const *sect = ".rodata";
        for (d = constdata_head(); d != NULL; d = d->datacdr)
            if (d->sort == LIT_ADCON && X86_PTRSIZE == 8)
                sect = ".data.rel.ro,\"aw\"";
        fprintf(as, "\n\t.section\t%s\n\t.p2align\t4\n.Lconst:\n", sect);
        asm_data(constdata_head());
    }
    if (data_size() != 0) {
        fputs("\n\t.data\n\t.p2align\t4\n.Ldata:\n", as);
        asm_data(data_head());
    }
    if (bss_size != 0) asm_bss();
    if (x86_negmask_used) {
        fputs("\n\t.section\t.rodata\n\t.p2align\t4\n"
              ".Lnegmaskf:\n\t.long\t0x80000000, 0, 0, 0\n"
              ".Lnegmaskd:\n\t.long\t0, 0x80000000, 0, 0\n", as);
    }
    if (x86_two32_used)
        fputs("\n\t.section\t.rodata\n\t.p2align\t3\n"
              ".Ltwo32:\n\t.long\t0, 0x41f00000\n", as);
    if (x86_two63_used)
        fputs("\n\t.section\t.rodata\n\t.p2align\t3\n"
              ".Ltwo63:\n\t.long\t0, 0x43e00000\n", as);
    /* Common blocks (tentative definitions), and weak references.   */
    for (x = obj_symlist; x != NULL; x = x->extcdr) {
        int32 flags = x->extflags;
        if (flags & (xr_defloc+xr_defext)) continue;
        if (!(flags & xr_code) && x->extoffset > 0) {
            fputs("\t.comm\t", as); pr_sym(x->extsym);
            fprintf(as, ", %ld, %d\n", (long)x->extoffset,
                    x->extoffset >= 16 ? 16 : x->extoffset >= 8 ? 8 : 4);
        } else if (flags & xr_weak) {
            fputs("\t.weak\t", as); pr_sym(x->extsym); fputc('\n', as);
        }
    }
    fputs("\t.section\t.note.GNU-stack,\"\",@progbits\n", as);
}

void asm_setregname(int regno, char const *name)
{   IGNORE(regno); IGNORE(name);
}

/* end of x86/asm.c */
