/*
 * simplify.h, version 3
 * Copyright (C) Codemist Ltd, 1988-1992.
 * Copyright (C) Acorn Computers Ltd., 1988-1990.
 * Copyright (C) Advanced RISC Machines Ltd., 1990-1992.
 * SPDX-Licence-Identifier: Apache-2.0
 */

/*
 * RCS $Revision$
 * Checkin $Date$
 * Revising $Author$
 */

#ifndef _simplify_h
#define _simplify_h

#ifndef _defs_LOADED
#  include "defs.h"
#endif

extern Expr *optimise0(Expr *e);

#define cg_optimise0(e) optimise0(e)

extern int32 mcrepofexpr(Expr *e);
extern int32 mcrepoftype(TypeExpr *t);

bool is_same(Expr *a,Expr *b);

bool returnsstructinregs_t(TypeExpr *t);
bool returnsstructinregs(Expr *fn);

#ifdef TARGET_HAS_SYSV_AMD64_ABI
/* The System V AMD64 psABI classes of the eightbytes of a struct.      */
#define AMD64_NONE      0
#define AMD64_INTEGER   1
#define AMD64_SSE       2
#define AMD64_MEMORY    3
/* The number of eightbytes (1 or 2) of a struct of type t passed in    */
/* registers, with their classes in cls[], or 0 if it is passed in      */
/* memory.                                                              */
extern int32 amd64_classify(TypeExpr *t, int32 cls[2]);
#endif

/* fields in mcrep result: */
#define MCR_SIZE_MASK    0x007fffff
#define MCR_SORT_MASK    0x07000000
#define MCR_ALIGN_DOUBLE 0x00800000
#define MCR_SORT_SHIFT   24

#define MCR_SORT_SIGNED   0x0000000
#define MCR_SORT_UNSIGNED 0x1000000
#define MCR_SORT_FLOATING 0x2000000
#define MCR_SORT_STRUCT   0x3000000
#define MCR_SORT_PLAIN    0x4000000  /* now probably defunct */

/* The next line has the effect of aligning locals to the same boundary */
/* as top-level variables.  Note the constant left operand of &&...     */
#define padtomcrep(a,r) padsize((a), \
   (int32)(alignof_double > alignof_toplevel_auto && ((r) & MCR_ALIGN_DOUBLE) ? \
                                     alignof_double : alignof_toplevel_auto))
#endif

/* end of simplify.h */
