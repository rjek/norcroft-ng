// Compound literals in the initialisers of automatic aggregates (and of
// other compound literals) with non-constant elements.
// RUN: %cc -std=c99 %s -o %t && %t
// REQUIRES: i386, i386-run

#include <stdio.h>
struct S { int a; long long b; };
struct T { int *p; long long q; int n; };
long long f1(long long x) { return ((struct S){ ((int[]){ 1 })[0], x * 2 }).b; }
long long f3(long long x) { long long a[2] = { ((long long[]){ x })[0], x + 1 }; return a[0] + a[1]; }
int f9(int x) { return ((struct S){ ((int[]){ x })[0], x }).a; }
long long f10(long long x) { struct T t = { (int[]){ 7, (int)x }, x << 33, ((struct S){ 3, x }).a }; return t.p[1] + t.q + t.n; }
long long f11(long long x) { struct S s = { ((struct S){ ((int[]){ (int)x, 2 })[1], ((long long[]){ x * 5 })[0] }).a, ((struct S){ 0, x - 1 }).b }; return s.a * 1000 + s.b; }
int main(void) { printf("%lld %lld %d %lld %lld\n", f1(21), f3(4), f9(6), f10(1), f11(10)); return 0; }

// CHECK: 42 9 6 8589934596 2009
