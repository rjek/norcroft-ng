/*
 * C compiler file meow/mcdpriv.h
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 */

#ifndef __mcdpriv_h
#define __mcdpriv_h 1

#include "mcdep.h"

#ifndef PCS_DEFAULTS
#  define PCS_DEFAULTS 0
#endif

extern int32 current_procnum;
extern List3 *label_values, *label_references;

/* obj.c */
extern Symstr *data_sym, *bss_sym;

#endif

/* end of meow/mcdpriv.h */
