/*
 * x86/x86ins.h -- the x86 back end's instruction list, shared between
 * gen.c (which builds it) and asm.c (which prints it).
 * SPDX-Licence-Identifier: Apache-2.0
 */

#ifndef _x86ins_h
#define _x86ins_h 1

#ifdef TARGET_IS_X86_64
#  define X86_PTRSIZE 8
#else
#  define X86_PTRSIZE 4
#endif

typedef enum {
    XO_NONE,
    XO_REG,         /* integer register reg, of size 1, 2, 4 or 8 bytes */
    XO_XMM,         /* xmm register reg                                 */
    XO_IMM,         /* $disp, or $sym+disp                              */
    XO_MEM,         /* [sym+disp+label](reg,index,scale); -1 for none,  */
                    /* or [sym+disp+label](%rip) if rip is set          */
    XO_LAB,         /* local label disp (a branch target)               */
    XO_SYM          /* symbol sym (a call target)                       */
} X86OpKind;

typedef struct X86Op {
    unsigned8 kind;
    unsigned8 size;
    unsigned8 star;         /* indirect jump/call target                */
    unsigned8 haslab;       /* XO_MEM: displacement includes label lab  */
    unsigned8 rip;          /* XO_MEM: relative to the program counter  */
    unsigned8 reloc;        /* XO_MEM: sym@GOTPCREL; XO_SYM: sym@PLT    */
    signed char reg, index;
    unsigned8 scale;
    int32 disp;
    int32 lab;
    Symstr const *sym;
} X86Op;

/* Special labels (for XO_MEM lab), printed by asm.c.                   */
#define XLAB_NEGMASKF   (-0x40000000L)
#define XLAB_NEGMASKD   (-0x40000001L)
#define XLAB_TWO32      (-0x40000002L)
#define XLAB_TWO63      (-0x40000003L)

typedef enum {
    XI_INSN,                /* mnem op[0], ..., op[nops-1]              */
    XI_LABEL,               /* op[0] (XO_LAB) is defined here           */
    XI_DATA,                /* data directive mnem op[0], or mnem       */
                            /* op[0]-op[1] if there are two operands    */
    XI_DIRECTIVE,           /* mnem is the complete text                */
    XI_DELETED              /* (removed after the event)                */
} X86InsKind;

typedef struct X86Ins {
    struct X86Ins *next;
    unsigned8 kind;
    unsigned8 nops;
    unsigned8 size;         /* if non-zero, the suffix for mnem: size   */
                            /* 1, 2, 4 or 8 bytes gives b, w, l or q    */
    unsigned8 frame;        /* sets up or takes down the frame          */
    char const *mnem;
    X86Op op[3];
} X86Ins;

extern X86Ins *x86_insns;           /* the current function's code      */
extern int32 x86_fnlabel;           /* its number, to make labels unique */
extern bool x86_negmask_used;
extern bool x86_two32_used;
extern bool x86_two63_used;

#endif

/* end of x86/x86ins.h */
