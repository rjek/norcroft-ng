// C23's _BitInt and <stdckdint.h> on a target of its own.
// RUN: %cc -std=c23 %s -c -o %t.o
// CHECK-ERR-NOT: Warning

#include <limits.h>
#include <stdckdint.h>
typedef unsigned _BitInt(4) u4;
static u4 table[3] = { 15, 16, 17 };
static _BitInt(40) big = 0x10000000000;
_Static_assert(BITINT_MAXWIDTH >= 64, "BITINT_MAXWIDTH");
_Static_assert(sizeof(_BitInt(9)) == 2, "container");
_Static_assert(_Generic(1uwb + 1uwb, unsigned _BitInt(1): 1, default: 0), "literal");
_Static_assert((u4)17 == 1, "wrap");
_Static_assert((_BitInt(5))31 == -1, "signed wrap");
int f(u4 *p, _BitInt(12) x, int y)
{   int r;
    (*p)++;
    table[x & 1] += y;
    return ckd_add(&r, x, y) + ckd_mul(&r, r, big) + (int)(x * big);
}
