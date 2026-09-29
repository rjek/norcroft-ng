/*
 * disass-meow.h: MEOW disassembler interface.
 * SPDX-Licence-Identifier: Apache-2.0
 */

#ifndef DISASS_MEOW_H
#define DISASS_MEOW_H

typedef enum { MEOW_D_BRANCH } meow_dis_type;

/* Called for a branch: offset is relative to the instruction, target is
 * the absolute address.  Writes the operand text into buf. */
typedef void (*meow_dis_cb)(meow_dis_type type, long offset,
                            unsigned long target, void *cb_arg, char *buf);

/* Disassemble one instruction into buf; returns its length in bytes. */
int disass_meow(unsigned short w, unsigned long addr, char *buf,
                void *cb_arg, meow_dis_cb cb);

#endif
