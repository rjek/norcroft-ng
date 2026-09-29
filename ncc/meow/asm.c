/*
 * C compiler file meow/asm.c
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * -S output in mas syntax.  The code buffer is disassembled, so what is
 * written is exactly what obj.c puts in an object.
 */

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "globals.h"
#include "mcdep.h"
#include "xrefs.h"
#include "store.h"
#include "codebuf.h"
#include "mcdpriv.h"
#include "builtin.h"
#include "version.h"
#include "errors.h"
#include "bind.h"
#include "disass-meow.h"

FILE *asmstream;

#ifndef NO_ASSEMBLER_OUTPUT

static bool headerdone;
static bool asm_error;

static void asm_blank(int32 n)
{   while (n-- > 0) fprintf(asmstream, "\n");
}

static int32 asm_padcol9(int32 n)
{
    while (n<9) fputc(' ',asmstream), n++;
    return n;
}

static void pr_string(int32 w)
{
  int i;
  union fudge { int32 i; unsigned char c[4]; } ff;
  ff.i = w;
  for (i = 0; i < 4; i++) {
    if (i) fputc(',', asmstream);
    fprintf(asmstream, "0x%02x", ff.c[i]);
  }
}

static void spr_asmname(char *buf, char const *s)
{   const char *s1 = s;
    char c;
    bool oddchars = NO;
    if (!isalpha(*s) && *s != '_')
        oddchars = YES;
    else
        while ((c = *s1++) != 0)
            if (!isalnum(c) && c != '_' && c != '$')
            {   oddchars=YES;
                break;
            }
    if (oddchars)
        sprintf(buf, "|%s|", s);
    else
        strcpy(buf, s);
}

static void pr_asmname(Symstr const *sym)
{   char const *s = sym == 0 ? (asm_error = 1, "?") : symname_(sym);
    char buf[256];
    spr_asmname(buf, s);
    fputs(buf, asmstream);
}

static Symstr *decode_external(int32 p)
{
    CodeXref *x;
    for (x = codexrefs; x!=NULL; x = x->codexrcdr)
        if (p == (x->codexroff & 0x00ffffff))
            return x->codexrsym;
    return 0;
}

static int32 destination_label;

static void disass_cb(meow_dis_type type, long offset, unsigned long target,
                      void *cb_arg, char *buf)
{
    IGNORE(type); IGNORE(offset); IGNORE(target); IGNORE(cb_arg);
    if (destination_label != -1)
        sprintf(buf, "F%ldL%ld", (long)current_procnum,
                (long)destination_label & 0xfffff);
    else {
        asm_error = YES;
        sprintf(buf, "?");
    }
}

static void decode_DC(int32 w)
{   int32 col = fprintf(asmstream, "DCD");
    col = asm_padcol9(col);
    fprintf(asmstream, "0x%.8lx", (long)w);
}

static void decode_DCA(Symstr *s, int32 w)
{   int32 col = fprintf(asmstream, "DCD");
    col = asm_padcol9(col);
    pr_asmname(s);
    if (w!=0) fprintf(asmstream, "%+ld", (long)w);
}

void display_assembly_code(Symstr const *name)
{   int32 q, ilen;
    List3 *w1, *w2;
    char buf[256];

    if (codep == 0) return;
    asm_blank(1);
    if (name != NULL) {
        if (StrEq(symname_(name), "main"))
            obj_symref(libentrypoint, xr_code, 0);
        pr_asmname(name);
        asm_blank(1);
    }
    label_values = (List3 *)dreverse((List *)label_values);
    label_references = (List3 *)dreverse((List *)label_references);
    for (w1=label_references; w1!=NULL; w1=(List3 *)cdr_(w1))
    {   for (w2=label_values; w2!=NULL; w2=(List3 *)cdr_(w2))
        {   int32 nn = w2->csr;
            if (nn < 0) continue;
            if (nn == (w1->csr & 0xfffff))
            {   w2->csr = nn | 0x80000000;
                break;
            }
        }
    }
    for (q=0; q < codep; q+=ilen)    /* q is a byte offset */
    {   const int32 f = code_flag_(q);
        int32 w;
        while (label_values != NULL && car_(label_values) < q)
            label_values = (List3 *)label_values->cdr;
        while (label_values != NULL && car_(label_values) == q) {
            if ((int32)label_values->csr < 0)
              fprintf(asmstream, "F%ldL%ld\n", (long)current_procnum,
                      (long)label_values->csr & 0x7fffffff);
            label_values = (List3 *)label_values->cdr;
        }
        destination_label = -1;
        while (label_references != NULL && car_(label_references) < q)
            label_references = (List3 *)label_references->cdr;
        if (label_references != NULL && car_(label_references) == q)
            destination_label = label_references->csr;
        if (f == LIT_OPCODE) {
            w = code_hword_(q);
            ilen = 2;
            if (w != 0)     /* zero is literal pool padding */
                disass_meow((unsigned short)w, (unsigned long)(codebase+q),
                            buf, NULL, disass_cb);
        } else if (f == LIT_BB || f == LIT_H) {
            w = code_hword_(q);
            ilen = 2;
        } else {
            w = code_inst_(q);
            ilen = 4;
        }
        fputs("        ", asmstream);
        switch (f)
        {
    case LIT_OPCODE:
            if (w == 0)
                fputs("DCW      0x0000", asmstream);
            else
                fputs(buf, asmstream);
            break;
    case LIT_STRING: {
            int32 col = fprintf(asmstream, "DCB");
            col = asm_padcol9(col);
            pr_string(w);
            break;
    }
    case LIT_BB: {
            unsigned char b[2];
            b[0] = (unsigned char)w;
            b[1] = (unsigned char)(w >> 8);
            fprintf(asmstream, "DCB      %#.2x,%#.2x", b[0], b[1]);
            break;
    }
    case LIT_H:
            fprintf(asmstream, "DCW      %#.4lx", (long)w);
            break;
    case LIT_NUMBER:
            decode_DC(w);
            break;
    case LIT_ADCON:
            decode_DCA(decode_external(codebase+q), w);
            break;
    case LIT_FPNUM:
    case LIT_FPNUM1:
    case LIT_FPNUM2:
            decode_DC(w);
            break;
    case LIT_INT64_1:
            fprintf(asmstream, "DCD      0x%.8lx, 0x%.8lx", (long)w,
                    (long)code_inst_(q+4));
            break;
    case LIT_INT64_2:
            continue;
    default:
            syserr("display_assembly_code %ld", (long)f);
            fprintf(asmstream, "?");
        }
        fprintf(asmstream, "\n");
    }
    asm_lablist = 0;
}

static char const *regnames[] = {
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "sp", "lr", "ir", "sr", "pc"
};

char const **regnamev = regnames;

void asm_setregname(int regno, char const *name) {
    if (headerdone) fprintf(asmstream, "%s RN %d\n", name, regno);
    regnamev[regno] = name;
}

void asm_header()
{
    asm_error = 0;
    headerdone = YES;
    fprintf(asmstream, "; generated by %s\n", CC_BANNER);
    asm_blank(1);
    fprintf(asmstream, "        AREA |C$$code|, CODE, READONLY\n");
    asm_blank(1);
    pr_asmname(bindsym_(codesegment));
    fprintf(asmstream, "\n");
}

static void asm_outextern()
{   ExtRef *x;
    for (x = obj_symlist; x != 0; x = x->extcdr)
    {   int32 flags = x->extflags;
        if (!(flags & xr_objflg) && !(flags & xr_defloc) && (flags & xr_defext))
        {   fprintf(asmstream, "        EXPORT ");
            pr_asmname(x->extsym);
            fprintf(asmstream, "\n");
        }
    }
    asm_blank(1);
    for (x = obj_symlist; x != 0; x = x->extcdr)
    {   int32 flags = x->extflags;
        if (!(flags & xr_objflg) && !(flags & xr_defloc) &&
            !(flags & xr_defext) && x->extsym != bindsym_(constdatasegment))
        {
            if (!(flags & xr_code) && (x->extoffset > 0)) continue;
            fprintf(asmstream, "        IMPORT ");
            pr_asmname(x->extsym);
            fprintf(asmstream, "\n");
        }
    }
}

typedef struct ExtRefList {
        struct ExtRefList *cdr;
        ExtRef *car;
} ExtRefList;

static void asm_pad(int32 len)
{
  asm_padcol9(1);
  fprintf(asmstream, "SPACE    %ld\n", (long)len);
}

static void asm_data(DataInit *p)
{
  for (; p != 0; p = p->datacdr) {
    IPtr rpt = p->rpt, sort = p->sort, len = p->len;
    union { unsigned32 l;
            unsigned16 w[2];
            unsigned8 b[4];
          } val;
    val.l = p->val;
    if (sort != LIT_LABEL) asm_padcol9(1);
    switch (sort) {
        case LIT_LABEL:
            pr_asmname((Symstr *)rpt);
            break;
        default:  syserr(syserr_asm_trailer, (long)sort);
        case LIT_BBBB:
            if (rpt == 1) {
                fprintf(asmstream, "DCB      %#.2x,%#.2x,%#.2x,%#.2x", val.b[0], val.b[1], val.b[2], val.b[3]);
                break;
            }
        case LIT_BBBX:
            if (rpt == 1) {
                fprintf(asmstream, "DCB      %#.2x,%#.2x,%#.2x", val.b[0], val.b[1], val.b[2]);
                break;
            }
        case LIT_BBX:
            if (rpt == 1) {
                fprintf(asmstream, "DCB      %#.2x,%#.2x", val.b[0], val.b[1]);
                break;
            }
        case LIT_BXXX:
            if (rpt == 1) {
                fprintf(asmstream, "DCB      %#.2x", val.b[0]);
                break;
            }
        case LIT_HH:
            if (rpt == 1) {
                fprintf(asmstream, "DCW      %#.4x,%#.4x", val.w[0], val.w[1]);
                break;
            }
        case LIT_HX:
            if (rpt == 1) {
                fprintf(asmstream, "DCW      %#.4x", val.w[0]);
                break;
            }
        case LIT_BBH:
            if (rpt == 1) {
                fprintf(asmstream, "DCB      %#.2x,%#.2x\n", val.b[0], val.b[1]);
                asm_padcol9(1);
                fprintf(asmstream, "DCW      %#.4x", val.w[1]);
                break;
            }
        case LIT_HBX:
            if (rpt == 1) {
                fprintf(asmstream, "DCW      %#.4x\n", val.w[0]);
                asm_padcol9(1);
                fprintf(asmstream, "DCB      %#.2x", val.b[2]);
                break;
            }
        case LIT_HBB:
            if (rpt == 1) {
                fprintf(asmstream, "DCW      %#.4x\n", val.w[0]);
                asm_padcol9(1);
                fprintf(asmstream, "DCB      %#.2x,%#.2x", val.b[2], val.b[3]);
                break;
            }
        case LIT_NUMBER:
            if (len != 4) syserr(syserr_asm_data, (long)len);
            if (rpt == 1) {
                fprintf(asmstream, "DCD      %#.8lx", (long)val.l);
            } else if (val.l == 0) {
                fprintf(asmstream, "SPACE    %ld", (long)rpt*len);
            }
            else syserr(syserr_asm_trailer1, (long)rpt, (long)val.l);
            break;
        case LIT_FPNUM:
        {   int32 *fp = ((FloatCon *)val.l) -> floatbin.irep;
            decode_DC(fp[0]);
            if (len == 8) fprintf(asmstream, "\n"),
                          asm_padcol9(1), decode_DC(fp[1]);
            break;
        }
        case LIT_ADCON:
            if (rpt != 1) syserr("asm_data adcon rpt");
            decode_DCA((Symstr *)len, val.l);
            break;
    }
    fprintf(asmstream, "\n");
  }
}

void asm_trailer()
{
  if (constdata_size() != 0) {
    asm_blank(1);
    fprintf(asmstream, "        AREA |C$$constdata|, DATA, READONLY\n");
    asm_blank(1);
    pr_asmname(bindsym_(constdatasegment));
    asm_blank(1);
    asm_data(constdata_head());
  }
  if (data_size() != 0)
  {
    asm_blank(1);
    fprintf(asmstream, "        AREA |C$$data|, DATA\n");
    asm_blank(1);
    asm_data(data_head());
  }
  if (bss_size != 0)
  { int32 n = 0;
    ExtRef *x = obj_symlist;
    ExtRefList *zisyms = NULL;
    asm_blank(1);
    fprintf(asmstream, "        AREA |C$$zinit|, BSS\n");
    asm_blank(1);
    for (; x != NULL; x = x->extcdr)
      if (x->extflags & xr_bss) {
        ExtRefList **prev = &zisyms;
        ExtRefList *p;
        for (; (p = *prev) != 0; prev = &cdr_(p))
          if (x->extoffset < car_(p)->extoffset) break;
        *prev = (ExtRefList *) syn_cons2(*prev, x);
      }
    for (; zisyms != NULL; zisyms = cdr_(zisyms))
    { x = car_(zisyms);
      if (x->extoffset != n) asm_pad(x->extoffset-n);
      n = x->extoffset;
      pr_asmname(x->extsym);
      fprintf(asmstream, "\n");
    }
    if (n != bss_size) asm_pad(bss_size-n);
  }
  { ExtRef *x;
    for (x = obj_symlist; x != NULL; x = x->extcdr)
    { int32 flags = x->extflags;
      if ((flags & (xr_defloc + xr_defext)) == 0) {
        Symstr *s = x->extsym;
        int32 len = x->extoffset;
        if (!(flags & xr_code) && (len > 0))
        { /* common data: give it a definition here */
          fputs("        EXPORT  ", asmstream);
          pr_asmname(s);
          asm_blank(1);
          fprintf(asmstream, "        AREA ");
          pr_asmname(s);
          fprintf(asmstream, ", BSS\n");
          pr_asmname(s);
          asm_blank(1);
          asm_pad(len);
        }
      }
    }
  }
  asm_blank(1);
  asm_outextern();
  asm_blank(1);
  fprintf(asmstream, "        END\n");
  headerdone = NO;
  if (asm_error) syserr(syserr_asm_confused);
}

#endif

/* end of meow/asm.c */
