// C99's _Bool: a byte, converting to 0 or 1 (also as a bit field, and
// with ++ and --), and <stdbool.h>.
// RUN: %cc -std=c99 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <stdbool.h>
struct S { _Bool f:1; bool g; int h:3; bool i:1; };
static _Bool sb = 256, sf = 0.5, sl = 0x100000000;
bool not(bool x) { return !x; }
int take(bool a, bool b) { return a * 10 + b; }
int main(void)
{   struct S s = {0};
    bool b = 0, arr[3] = { 1, 0, 5 };
    int v = 2;
    s.f = v; s.g = v; s.i = 4; s.h = 1;
    printf("%d %d %d %d %zu %zu\n", s.f, s.g, s.h, s.i, sizeof(bool), sizeof arr);
    b++; printf("%d ", b); b++; printf("%d ", b); b--; printf("%d ", b); b--; printf("%d\n", b);
    s.f = 0; s.f |= 2; s.g = 0; s.g += 4;
    printf("%d %d %d %d %d %d %d\n", s.f, s.g, sb, sf, sl, not(3), take(7, 0));
    return 0;
}

// CHECK: 1 1 1 1 1 3
// CHECK: 1 1 0 1
// CHECK: 1 1 1 1 1 0 10
