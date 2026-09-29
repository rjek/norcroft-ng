// Other C99 things: __func__, flexible array members, wide and ordinary
// string concatenation, a trailing comma in an enum, static and
// qualifiers in array parameters, restrict, // comments (even with
// -strict) and the infinities of glibc's HUGE_VAL and INFINITY.
// RUN: %cc -std=c99 -strict %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>
enum E { A, B, };
struct F { int n; double d[]; };
const char *who(void) { return __func__; }
int f(int a[static 3], int b[const], int *restrict c) { return a[2] + b[0] + *c; }
int main(void)
{   const wchar_t *w = L"ab" "cd";
    struct F *fl = (struct F *)malloc(sizeof(struct F) + 2 * sizeof(double));
    int x[3] = { 1, 2, 3 };
    fl->d[1] = 2.5;
    printf("%s %s %zu %zu %zu %g %d %d\n", __func__, who(), sizeof(__func__),
           wcslen(w), sizeof(struct F), fl->d[1], B, f(x, x, x)); // comment
    printf("%g %g %d\n", HUGE_VAL, -INFINITY, isinf(INFINITY) != 0);
    return 0;
}

// CHECK: main who 5 4 8 2.5 1 5
// CHECK: inf -inf 1
