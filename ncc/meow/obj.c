/*
 * C compiler file meow/obj.c
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Object file interface for MEOW.  Symbols are tracked here as every
 * backend must; object output itself is not yet supported, so the code is
 * written as assembler with -S and assembled with mas.
 */

#include <string.h>

#include "globals.h"
#include "mcdep.h"
#include "mcdpriv.h"
#include "xrefs.h"
#include "store.h"
#include "codebuf.h"
#include "errors.h"

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

void obj_init(void)
{
    obj_symcount = 0;
    obj_symlist = NULL;
    codexrefs = NULL;
    dbgxrefs = NULL;
}

void obj_header(void)
{
    cc_fatalerr(obj_fatalerr_noobj);
}

void obj_codewrite(Symstr *name)
{
    IGNORE(name);
}

void obj_trailer(void)
{
}

void obj_common_start(Symstr *name)
{
    IGNORE(name);
}

void obj_common_end(void)
{
}

/* end of meow/obj.c */
