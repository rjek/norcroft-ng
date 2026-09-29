/*
 * x86/mcdpriv.h -- private interfaces within the x86 back end.
 * SPDX-Licence-Identifier: Apache-2.0
 */

#ifndef __mcdpriv_h
#define __mcdpriv_h 1

#include "mcdep.h"
#ifndef JOPCODEDEF_ONLY
#include "codebuf.h"
#endif

/* Calling-standard options (-apcs is not meaningful on x86).           */
#define PCS_NOFP            0x01    /* omit frame pointer               */
#define PCS_NOSTACKCHECK    0x02
#define PCS_REENTRANT       0x04

extern int32 pcs_flags;

/* Hardware register numbers (as used in instruction encodings), named  */
/* after the 32-bit registers.                                          */
#define X86_EAX 0
#define X86_ECX 1
#define X86_EDX 2
#define X86_EBX 3
#define X86_ESP 4
#define X86_EBP 5
#define X86_ESI 6
#define X86_EDI 7
#define X86_R8  8               /* x86-64 only                          */
#define X86_R9  9
#define X86_R10 10
#define X86_R11 11
#define X86_R12 12
#define X86_R13 13
#define X86_R14 14
#define X86_R15 15

/* Block moves and clears of up to this many bytes are done in-line     */
/* (with r11 as a scratch register), and longer ones with rep movs.     */
#define SMALL_BLOCK 64

/* Interface between gen.c and asm.c.                                   */
extern void x86_asm_function(Symstr const *name);

#endif

/* end of x86/mcdpriv.h */
