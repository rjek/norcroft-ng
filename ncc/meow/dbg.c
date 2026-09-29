/*
 * C compiler file meow/dbg.c
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * The debugger interface, which MEOW does not (yet) implement.
 */

#include "globals.h"
#include "mcdep.h"
#include "xrefs.h"

char dbg_name[] = "none";
int usrdbgmask;

int32 dbg_tablesize(void) { return 0; }
int32 dbg_tableindex(int32 dt_number) { IGNORE(dt_number); return 0; }
void *dbg_notefileline(FileLine fl) { IGNORE(fl); return NULL; }
void dbg_addcodep(void *debaddr, int32 codeaddr) { IGNORE(debaddr); IGNORE(codeaddr); }
bool dbg_scope(BindListList *newbl, BindListList *old) { IGNORE(newbl); IGNORE(old); return NO; }
void dbg_final_src_codeaddr(int32 a, int32 b) { IGNORE(a); IGNORE(b); }
void dbg_topvar(Symstr *name, int32 addr, TypeExpr *t, int stgclass, FileLine fl)
{ IGNORE(name); IGNORE(addr); IGNORE(t); IGNORE(stgclass); IGNORE(fl); }
void dbg_type(Symstr *name, TypeExpr *t, FileLine fl) { IGNORE(name); IGNORE(t); IGNORE(fl); }
void dbg_proc(Symstr *name, TypeExpr *t, bool ext, FileLine fl)
{ IGNORE(name); IGNORE(t); IGNORE(ext); IGNORE(fl); }
void dbg_locvar(Binder *name, FileLine fl) { IGNORE(name); IGNORE(fl); }
void dbg_locvar1(Binder *name) { IGNORE(name); }
void dbg_commblock(Binder *name, SynBindList *members, FileLine fl)
{ IGNORE(name); IGNORE(members); IGNORE(fl); }
void dbg_enterproc(void) {}
void dbg_bodyproc(void) {}
void dbg_return(int32 addr) { IGNORE(addr); }
void dbg_xendproc(FileLine fl) { IGNORE(fl); }
void dbg_init(void) {}
void dbg_finalise(void) {}
void dbg_setformat(char const *format) { IGNORE(format); }
bool dbg_debugareaexists(char const *name) { IGNORE(name); return NO; }
Symstr *obj_notedebugarea(char const *name) { IGNORE(name); return NULL; }
void obj_startdebugarea(char const *name) { IGNORE(name); }
void obj_enddebugarea(char const *name, DataXref *relocs) { IGNORE(name); IGNORE(relocs); }
void dbg_writedebug(void) {}
void obj_writedebug(void const *p, int32 n) { IGNORE(p); IGNORE(n); }
int32 local_fpaddress(Binder const *b) { IGNORE(b); return 0; }
RealRegister local_fpbase(Binder const *b) { IGNORE(b); return R_NOFPREG; }

/* end of meow/dbg.c */
