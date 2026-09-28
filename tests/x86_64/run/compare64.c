// 64-bit compares, range checks, and conversions of 64-bit constants to double.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <limits.h>
#define MAXARG_Bx 131071
#define OFFSET_sBx 65535
#define fitsBx(i) (-OFFSET_sBx <= (i) && (i) <= MAXARG_Bx - OFFSET_sBx)
int fits(long i) { return fitsBx(i); }
int le(long i, int k) { return i <= k; }
int ule(unsigned long i, unsigned k) { return i <= k; }
double dmin(void) { return (double)LONG_MIN; }
double dmax(void) { return (double)LONG_MAX; }
double dbig(void) { return (double)(1L << 40); }
double ubig(void) { return (double)0xfffffffffffffff0UL; }
int main(void)
{   volatile long big = 1L << 33;
    printf("%d %d %d %d\n", fits(big), fits(5), fits(-70000), fits(65536));
    printf("%d %d %d\n", le(big, 5), le(-big, 5), ule(big, 5u));
    printf("%g %g %g %g\n", dmin(), dmax(), dbig(), ubig());
    printf("%d\n", 4294967296.0 >= (double)LONG_MIN && 4294967296.0 < -(double)LONG_MIN);
    return 0;
}

// CHECK: 0 1 0 1
// CHECK: 0 1 0
// CHECK: -9.22337e+18 9.22337e+18 1.09951e+12 1.84467e+19
// CHECK: 1
