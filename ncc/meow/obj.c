/*
 * C compiler file meow/obj.c
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * ELF relocatable output for MEOW, written with the melf library from the
 * MEOW repository.  Code, initialised data, read-only data and BSS become
 * .text, .data, .rodata and .bss; every reference is an ABS32 relocation
 * against a symbol, which is all the code generator ever emits.
 */

#include <string.h>

#include "globals.h"
#include "mcdep.h"
#include "mcdpriv.h"
#include "xrefs.h"
#include "store.h"
#include "codebuf.h"
#include "builtin.h"
#include "errors.h"
#include "melf.h"

FILE *objstream;
ExtRef *obj_symlist;
CodeXref *codexrefs;
DataXref *dbgxrefs;
Symstr *data_sym, *bss_sym;

static int32 obj_symcount;

int32 obj_symref(Symstr *s, int flags, int32 loc)
{   ExtRef *x;
    if ((x = symext_(s)) == 0)
    {   x = (ExtRef *) GlobAlloc(SU_Xsym, sizeof(ExtRef));
        x->extcdr = obj_symlist,
          x->extsym = s,
          x->extindex = obj_symcount++,
          x->extflags = 0,
          x->extoffset = 0;
        obj_symlist = symext_(s) = x;
    }
    if (flags & (xr_defloc+xr_defext)) {
        if (!(x->extflags & (xr_defloc+xr_defext)))
            x->extflags &= ~(xr_code+xr_data);
    } else if (x->extflags & (xr_defloc+xr_defext))
        flags &= ~(xr_code+xr_data);
    x->extflags |= flags;
    if (flags & xr_defloc+xr_defext)
        x->extoffset = loc;
    else if ((loc > 0) && !(flags & xr_code) &&
               !(x->extflags & xr_defloc+xr_defext))
    {   if (loc > x->extoffset) x->extoffset = loc;
    }
    return ((x->extflags & (xr_defloc+xr_defext)) && (x->extflags & xr_code) ?
            x->extoffset : -1);
}

int32 obj_symdef(Symstr *s, int flags, int32 loc)
{
    symext_(s) = NULL;
    return obj_symref(s, flags, loc);
}

/* ---- output sections --------------------------------------------------- */

typedef struct {
    unsigned8 *p;
    uint32_t n, cap;
} Buffer;

typedef struct Reloc Reloc;
struct Reloc {
    Reloc *next;
    Buffer *sec;
    uint32_t off;
    Symstr *sym;
    int32 addend;
};

static Buffer text, data, rodata;
static Reloc *relocs;

static void buf_reset(Buffer *b)
{
    free(b->p);
    b->p = NULL;
    b->n = b->cap = 0;
}

static void buf_bytes(Buffer *b, const void *v, uint32_t n)
{
    if (b->n + n > b->cap) {
        b->cap = (b->n + n) * 2 + 256;
        b->p = realloc(b->p, b->cap);
        if (b->p == NULL) cc_fatalerr(obj_fatalerr_io_object);
    }
    memcpy(b->p + b->n, v, n);
    b->n += n;
}

static void buf_word(Buffer *b, unsigned32 w)
{
    unsigned8 v[4];
    v[0] = (unsigned8)w;
    v[1] = (unsigned8)(w >> 8);
    v[2] = (unsigned8)(w >> 16);
    v[3] = (unsigned8)(w >> 24);
    buf_bytes(b, v, 4);
}

static void buf_pad(Buffer *b, uint32_t n)
{
    static const unsigned8 zero[16];
    while (n > 0) {
        uint32_t k = n < sizeof zero ? n : sizeof zero;
        buf_bytes(b, zero, k);
        n -= k;
    }
}

static void add_reloc(Buffer *sec, uint32_t off, Symstr *sym, int32 addend)
{
    Reloc *r = (Reloc *)GlobAlloc(SU_Xref, sizeof(Reloc));
    if (sym == NULL) syserr(syserr_obj_codereloc, (long)off);
    r->next = relocs;
    r->sec = sec;
    r->off = off;
    r->sym = sym;
    r->addend = addend;
    relocs = r;
}

static Symstr *code_external(int32 p)
{
    CodeXref *x;
    for (x = codexrefs; x != NULL; x = x->codexrcdr)
        if (p == (x->codexroff & 0x00ffffff))
            return x->codexrsym;
    return NULL;
}

void obj_init(void)
{
    obj_symcount = 0;
    obj_symlist = NULL;
    codexrefs = NULL;
    dbgxrefs = NULL;
    relocs = NULL;
    buf_reset(&text);
    buf_reset(&data);
    buf_reset(&rodata);
}

void obj_header(void)
{
}

void obj_codewrite(Symstr *name)
{
    int32 q;

    IGNORE(name);
    if (codep == 0) return;
    if ((int32)text.n != codebase) syserr(syserr_obj_codereloc, (long)codebase);
    for (q = 0; q < codep; q += 2) {
        unsigned16 hw = code_hword_(q);
        unsigned8 v[2];

        if ((q & 3) == 0 && code_flag_(q) == LIT_ADCON)
            add_reloc(&text, text.n, code_external(codebase + q), code_inst_(q));
        v[0] = (unsigned8)hw;
        v[1] = (unsigned8)(hw >> 8);
        buf_bytes(&text, v, 2);
    }
}

/* Serialise a DataInit list, as asm_data prints it. */
static void write_data(Buffer *b, DataInit *p)
{
    for (; p != NULL; p = p->datacdr) {
        IPtr rpt = p->rpt, sort = p->sort, len = p->len;
        unsigned32 val = (unsigned32)p->val;

        switch (sort) {
        case LIT_LABEL:
            break;
        case LIT_BBBB: case LIT_BBBX: case LIT_BBX: case LIT_BXXX:
        case LIT_HH: case LIT_HX: case LIT_BBH: case LIT_HBX: case LIT_HBB:
        {   unsigned8 v[4];
            v[0] = (unsigned8)val;
            v[1] = (unsigned8)(val >> 8);
            v[2] = (unsigned8)(val >> 16);
            v[3] = (unsigned8)(val >> 24);
            while (rpt-- > 0) buf_bytes(b, v, (uint32_t)len);
            break;
        }
        case LIT_NUMBER:
            if (len != 4) syserr(syserr_asm_data, (long)len);
            while (rpt-- > 0) buf_word(b, val);
            break;
        case LIT_ADCON:
            if (rpt != 1) syserr("obj_data adcon rpt");
            add_reloc(b, b->n, (Symstr *)len, (int32)val);
            buf_word(b, val);
            break;
        case LIT_FPNUM:
        {   FloatCon *fc = (FloatCon *)p->val;
            while (rpt-- > 0) {
                if (len == 8) {
                    buf_word(b, fc->floatbin.db.lsd);
                    buf_word(b, fc->floatbin.db.msd);
                } else
                    buf_word(b, (unsigned32)fc->floatbin.irep[0]);
            }
            break;
        }
        default:
            syserr(syserr_asm_trailer, (long)sort);
        }
    }
}

typedef struct {
    int sec;
    Buffer *buf;
} Placement;

static bool is_segment_symbol(Symstr *s)
{
    return s == bindsym_(codesegment) || s == bindsym_(datasegment) ||
           s == bindsym_(constdatasegment);
}

/* Where a symbol lives: section index and offset, from its ExtRef flags.
 * Returns NO for an undefined external. */
static bool place_symbol(ExtRef *x, int text_sec, int data_sec, int rodata_sec,
                         int bss_sec, uint32_t *bss_top, int *sec, uint32_t *off)
{
    int32 flags = x->extflags;
    Symstr *s = x->extsym;

    *off = (uint32_t)x->extoffset;
    if ((flags & (xr_defloc | xr_defext)) != 0) {
        if ((flags & xr_bss) != 0) *sec = bss_sec;
        else if ((flags & xr_constdata) != 0) *sec = rodata_sec;
        else if ((flags & xr_data) != 0) *sec = data_sec;
        else if ((flags & (xr_code | xr_dataincode)) != 0) *sec = text_sec;
        else *sec = MELF_SHN_ABS;
        return YES;
    }
    if (s == bindsym_(codesegment)) { *sec = text_sec; *off = 0; return YES; }
    if (s == bindsym_(datasegment)) { *sec = data_sec; *off = 0; return YES; }
    if (s == bindsym_(constdatasegment)) { *sec = rodata_sec; *off = 0; return YES; }
    if ((flags & xr_code) == 0 && x->extoffset > 0) {
        /* a tentative definition with no initialiser: give it BSS */
        *bss_top = (*bss_top + 3) & ~3u;
        *sec = bss_sec;
        *off = *bss_top;
        *bss_top += (uint32_t)x->extoffset;
        return YES;
    }
    return NO;
}

void obj_trailer(void)
{
    struct melf *e = melf_new(MELF_ET_REL);
    int text_sec, data_sec, rodata_sec, bss_sec;
    int *symidx;
    uint32_t bss_top = (uint32_t)bss_size;
    ExtRef *x;
    Reloc *r;
    int pass;

    write_data(&rodata, constdata_head());
    write_data(&data, data_head());
    if ((int32)rodata.n != constdata_size()) syserr(syserr_asm_trailer, (long)rodata.n);
    if ((int32)data.n != data_size()) syserr(syserr_asm_trailer, (long)data.n);

    text_sec = melf_add_section(e, ".text", MELF_SHT_PROGBITS,
                                MELF_SHF_ALLOC | MELF_SHF_EXECINSTR,
                                text.p, text.n, 4);
    rodata_sec = melf_add_section(e, ".rodata", MELF_SHT_PROGBITS,
                                  MELF_SHF_ALLOC, rodata.p, rodata.n, 4);
    data_sec = melf_add_section(e, ".data", MELF_SHT_PROGBITS,
                                MELF_SHF_ALLOC | MELF_SHF_WRITE,
                                data.p, data.n, 4);
    /* the BSS size is not final until the commons are placed: patched below */
    bss_sec = melf_add_section(e, ".bss", MELF_SHT_NOBITS,
                               MELF_SHF_ALLOC | MELF_SHF_WRITE, NULL, 0, 4);

    symidx = (int *)GlobAlloc(SU_Xsym, (obj_symcount + 1) * sizeof(int));
    /* locals in the first pass, globals in the second, as ELF wants */
    for (pass = 0; pass < 2; pass++) {
        for (x = obj_symlist; x != NULL; x = x->extcdr) {
            int32 flags = x->extflags;
            bool local = (flags & xr_defloc) != 0 ||
                         ((flags & xr_defext) == 0 && is_segment_symbol(x->extsym));
            int sec;
            uint32_t off;
            unsigned type;

            if ((pass == 0) != local) continue;
            if (place_symbol(x, text_sec, data_sec, rodata_sec, bss_sec,
                             &bss_top, &sec, &off) == NO) {
                symidx[x->extindex] = melf_add_symbol(e, symname_(x->extsym), 0,
                        MELF_STB_GLOBAL, MELF_STT_NOTYPE, MELF_SHN_UNDEF);
                continue;
            }
            if (is_segment_symbol(x->extsym)) type = MELF_STT_NOTYPE;
            else if (sec == text_sec && (flags & xr_dataincode) == 0) type = MELF_STT_FUNC;
            else type = MELF_STT_OBJECT;
            symidx[x->extindex] = melf_add_symbol(e, symname_(x->extsym), off,
                    local ? MELF_STB_LOCAL : MELF_STB_GLOBAL, type, sec);
        }
    }
    e->sections[bss_sec].size = bss_top;

    for (r = relocs; r != NULL; r = r->next) {
        ExtRef *sx = symext_(r->sym);
        int sec = r->sec == &text ? text_sec : r->sec == &data ? data_sec : rodata_sec;
        if (sx == NULL) syserr(syserr_obj_codereloc, (long)r->off);
        melf_add_rela(e, sec, r->off, symidx[sx->extindex], R_MEOW_ABS32, r->addend);
    }

    if (melf_fwrite(e, objstream) == false) cc_fatalerr(obj_fatalerr_io_object);
    melf_free(e);
    obj_init();
}

void obj_common_start(Symstr *name)
{
    IGNORE(name);
}

void obj_common_end(void)
{
}

/* end of meow/obj.c */
