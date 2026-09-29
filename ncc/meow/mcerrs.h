/*
 * C compiler file meow/mcerrs.h
 * Copyright (C) Codemist Ltd., 1988-94.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * Error messages for the MEOW backend, merged into errors.h by genhdrs.
 */

#ifndef NLS                     /* if NLS, miperrs will have included tags */

%O  /* Ordinary error messages - mapped onto numeric codes */

#define misc_disaster_banner   "\n\
Internal inconsistency: either resource shortage or compiler fault. If you\n\
cannot alter your program to avoid this failure, please contact your supplier\n"

#define gen_err_swi "MEOW has no SWI instruction"
#define gen_err_irq "%s cannot handle __irq functions"
#define gen_err_inlineasm "MEOW has no inline assembler"
#define gen_err_unaligned "unaligned access is not supported on MEOW"
#define asm_err_corrupted_reg   "R%d corrupted but possibly reused later. This code may not work correctly"
#define obj_err_common "repeated common block $r"
#define obj_err_common1 "common block $r too small"
#define obj_err_common2 "common block $r too large"
#define obj_fatalerr_noobj "object output is not supported for MEOW; use -S and assemble with mas"

#endif                          /* ndef NLS - syserrs are not tags */

%S  /* System failure messages - error text not preserved */

#define syserr_displacement "displacement out of range %ld"
#define syserr_local_base "local_base %lx"
#define syserr_local_addr "local_address %lx"
#define syserr_show_inst_dir "show_instruction(%#lx)"
#define syserr_setsp_confused "SETSP confused %ld!=%ld %ld"
#define syserr_asm_trailer "asm_trailer(%ld)"
#define syserr_asm_data "asm_data len=%ld"
#define syserr_asm_trailer1 "asm_trailer(%ldF%ld)"
#define syserr_asm_confused "asm_trailer confused"
#define syserr_meow_reg "bad register %ld in %s"
#define syserr_meow_litref "literal reference not in code"

/* end of meow/mcerrs.h */
