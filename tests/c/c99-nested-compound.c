// Compound literals in the initialisers of automatic aggregates (and of
// other compound literals) with non-constant elements.
// RUN: %cc -std=c99 %s -c -o %t.o
// CHECK-ERR-NOT: rror

struct S { int a; long long b; };
struct T { int *p; long long q; int n; };
long long f1(long long x) { return ((struct S){ ((int[]){ 1 })[0], x * 2 }).b; }
long long f3(long long x) { long long a[2] = { ((long long[]){ x })[0], x + 1 }; return a[0] + a[1]; }
long long f10(long long x) { struct T t = { (int[]){ 7, (int)x }, x << 33, ((struct S){ 3, x }).a }; return t.p[1] + t.q + t.n; }
