/*
 * disass-meow.c: MEOW disassembler for the compiler's -S output.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * A branch target goes through the callback with D_BORBL so that asm.c
 * can print a label in place of the address.  Everything else is printed
 * literally, in mas syntax.
 */

#include <stdio.h>
#include <string.h>

#include "host.h"
#include "meow_isa.h"
#include "disass-meow.h"

static const char *const regnames[16] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
    "sp", "lr", "ir", "sr", "pc"
};

static const char *const condnames[16] = {
    "EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
    "HI", "LS", "GE", "LT", "GT", "LE", "", "NV"
};

static const char *reg(unsigned r, unsigned alt, char *tmp)
{
    sprintf(tmp, "%s%s", alt != 0 ? "a" : "", regnames[r & 15]);
    return tmp;
}

static int bit_of(unsigned bit)
{
    return (int)bit;
}

int disass_meow(unsigned short w, unsigned long addr, char *buf,
                void *cb_arg, meow_dis_cb cb)
{
    char a[8];
    char b[8];
    const char *m;

    switch (meow_enc_of(w)) {
    case MEOW_ENC_B: {
        unsigned cond = MEOW_B_COND(w);
        long off = (long)MEOW_B_OFF_S(w) * 2;

        if (cond == 15) {
            sprintf(buf, "BNV      #%ld", off);
        } else {
            char *p = buf + sprintf(buf, "B%-8s", condnames[cond]);

            if (cb != NULL) {
                cb(MEOW_D_BRANCH, off, addr + (unsigned long)off, cb_arg, p);
            } else {
                sprintf(p, "0x%08lx", addr + (unsigned long)off);
            }
        }
        return 2;
    }
    case MEOW_ENC_ADD3:
    case MEOW_ENC_SUB3:
        m = (w & 0x4000) != 0 ? "SUB" : "ADD";
        if (MEOW_ADD3_IMM(w) == 0) {
            sprintf(buf, "%-8s %s, %s", m, regnames[MEOW_ADD3_RD(w)],
                    regnames[MEOW_ADD3_RS(w)]);
        } else {
            sprintf(buf, "%-8s %s, %s, #%u", m, regnames[MEOW_ADD3_RD(w)],
                    regnames[MEOW_ADD3_RS(w)], MEOW_ADD3_IMM(w));
        }
        return 2;
    case MEOW_ENC_ADD8:
    case MEOW_ENC_SUB8:
        m = (w & 0x4000) != 0 ? "SUB" : "ADD";
        sprintf(buf, "%-8s %s, #%u", m, regnames[MEOW_ADD8_RD(w)],
                MEOW_ADD8_IMM(w));
        return 2;
    case MEOW_ENC_CMPI:
        sprintf(buf, "%-8s %s, #%d", "CMP", regnames[MEOW_CMPI_RN(w)],
                (int)MEOW_CMPI_IMM_S(w));
        return 2;
    case MEOW_ENC_CMPR:
        sprintf(buf, "%-8s %s, %s", "CMP",
                reg(MEOW_CMPR_RN(w), MEOW_CMPR_BN(w), a),
                reg(MEOW_CMPR_RM(w), MEOW_CMPR_BM(w), b));
        return 2;
    case MEOW_ENC_TST:
        sprintf(buf, "%-8s %s, #0x%x", "TST",
                reg(MEOW_TST_RN(w), MEOW_TST_BN(w), a),
                1u << bit_of(MEOW_TST_BIT(w)));
        return 2;
    case MEOW_ENC_MOV: {
        char mn[8];

        sprintf(mn, "MOV%s%s", MEOW_MOV_BSW(w) != 0 ? "B" : "",
                MEOW_MOV_HSW(w) != 0 ? "W" : "");
        sprintf(buf, "%-8s %s, %s", mn,
                reg(MEOW_MOV_RD(w), MEOW_MOV_BD(w), a),
                reg(MEOW_MOV_RS(w), MEOW_MOV_BS(w), b));
        return 2;
    }
    case MEOW_ENC_LDI:
        sprintf(buf, "%-8s #%d", "LDI", (int)MEOW_LDI_IMM_S(w));
        return 2;
    case MEOW_ENC_SHI:
        m = MEOW_SHI_ROT(w) != 0 ? (MEOW_SHI_LEFT(w) != 0 ? "ROL" : "ROR")
                                 : (MEOW_SHI_LEFT(w) != 0 ? "LSL" : "LSR");
        sprintf(buf, "%-8s %s, #%u", m, regnames[MEOW_SHI_RD(w)],
                MEOW_SHI_IMM(w));
        return 2;
    case MEOW_ENC_SHR:
        m = MEOW_SHR_ROT(w) != 0 ? (MEOW_SHR_LEFT(w) != 0 ? "ROL" : "ROR")
                                 : (MEOW_SHR_LEFT(w) != 0 ? "LSL" : "LSR");
        sprintf(buf, "%-8s %s, %s", m, regnames[MEOW_SHR_RD(w)],
                regnames[MEOW_SHR_RS(w)]);
        return 2;
    case MEOW_ENC_ASRI:
        sprintf(buf, "%-8s %s, #%u", "ASR", regnames[MEOW_ASRI_RD(w)],
                MEOW_ASRI_IMM(w));
        return 2;
    case MEOW_ENC_ASRR:
        sprintf(buf, "%-8s %s, %s", "ASR", regnames[MEOW_ASRR_RD(w)],
                regnames[MEOW_ASRR_RS(w)]);
        return 2;
    case MEOW_ENC_ADDSI:
        sprintf(buf, "%-8s %s, #%u", MEOW_ADDSI_SUB(w) != 0 ? "SUBS" : "ADDS",
                regnames[MEOW_ADDSI_RD(w)], MEOW_ADDSI_IMM(w));
        return 2;
    case MEOW_ENC_ADDSR:
        sprintf(buf, "%-8s %s, %s", MEOW_ADDSR_SUB(w) != 0 ? "SUBS" : "ADDS",
                regnames[MEOW_ADDSR_RD(w)], regnames[MEOW_ADDSR_RS(w)]);
        return 2;
    case MEOW_ENC_SPMEM:
        sprintf(buf, "%-8s %s, [sp, #%u]", MEOW_SPMEM_STORE(w) != 0 ? "STR" : "LDR",
                regnames[MEOW_SPMEM_RV(w)], 4 * MEOW_SPMEM_IMM(w));
        return 2;
    case MEOW_ENC_BITR:
    case MEOW_ENC_BITI: {
        static const char *const names[2][4] = {
            { "MVN", "AND", "ORR", "EOR" },
            { NULL, "BIC", "ORN", "EON" }
        };

        m = names[MEOW_BITR_INV(w) & 1][MEOW_BITR_OP(w) & 3];
        if (m == NULL) {
            break;
        }
        if (meow_enc_of(w) == MEOW_ENC_BITR) {
            sprintf(buf, "%-8s %s, %s", m, regnames[MEOW_BITR_RD(w)],
                    regnames[MEOW_BITR_RS(w)]);
        } else {
            sprintf(buf, "%-8s %s, #0x%x", m, regnames[MEOW_BITI_RD(w)],
                    1u << bit_of(MEOW_BITI_BIT(w)));
        }
        return 2;
    }
    case MEOW_ENC_MEM: {
        static const char *const suffix[2][2] = { { "B", "" }, { "HH", "H" } };
        static const unsigned bytes[2][2] = { { 1, 4 }, { 2, 2 } };
        unsigned half = MEOW_MEM_HALF(w);
        unsigned hilo = MEOW_MEM_HILO(w);
        char mn[8];
        const char *rv = regnames[MEOW_MEM_RV(w)];
        const char *ra = regnames[MEOW_MEM_RA(w)];

        sprintf(mn, "%s%s", MEOW_MEM_STORE(w) != 0 ? "STR" : "LDR",
                suffix[half][hilo]);
        if (MEOW_MEM_WB(w) == 0 && MEOW_MEM_DIR(w) == 0) {
            sprintf(buf, "%-8s %s, [%s]", mn, rv, ra);
        } else if (MEOW_MEM_WB(w) == 0) {
            sprintf(buf, "%-8s %s, [%s, #-%u]!", mn, rv, ra, bytes[half][hilo]);
        } else {
            sprintf(buf, "%-8s %s, [%s], #%s%u", mn, rv, ra,
                    MEOW_MEM_DIR(w) != 0 ? "" : "-", bytes[half][hilo]);
        }
        return 2;
    }
    default:
        break;
    }
    sprintf(buf, "%-8s 0x%04x", "DCW", w);
    return 2;
}

/* end of disass-meow.c */
