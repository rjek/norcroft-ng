// C23's _BitInt(N): wrapping on conversion and arithmetic, the types of
// results and literals, initialisers, and _Generic.
// RUN: %cc -std=c23 %s -o %t && %t
// REQUIRES: i386, i386-run

#include <stdio.h>
#include <limits.h>
typedef unsigned _BitInt(4) u4;
typedef _BitInt(4) s4;
struct S { unsigned _BitInt(4) a; _BitInt(12) b; unsigned _BitInt(40) c; };
static u4 gu = 17;
static s4 gs = 9;
static unsigned _BitInt(40) g40 = -1;
static _BitInt(33) g33 = 0x100000000;
static struct S gst = { 20, 2048, -1 };
static u4 garr[3] = { 15, 16, 17 };
constexpr unsigned _BitInt(6) K = 63;
unsigned _BitInt(12) addu(unsigned _BitInt(12) a, unsigned _BitInt(12) b) { return a + b; }
_BitInt(12) adds(_BitInt(12) a, _BitInt(12) b) { return a + b; }
unsigned _BitInt(9) ret(int x) { return x; }
#define P(x) printf("%llx ", (unsigned long long)(x))
int main(void)
{   u4 x = 15; s4 y = 7;
    unsigned _BitInt(40) z = 0;
    _BitInt(48) w = 0x7fffffffffffwb;
    struct S s = gst, *p = &s;
    u4 *q = &garr[0];
    volatile int v = 20, vw = 2048;
    volatile long long ll = -1;
    printf("%d\n", BITINT_MAXWIDTH >= 64);
    /* static initialisers, wrapped */
    P(gu); P(gs); P(g40); P(g33); P(gst.a); P(gst.b); P(gst.c); P(garr[1]); P(garr[2]); puts("");
    /* increments, compound assignment and arithmetic wrap */
    x++; P(x); x--; P(x); x += 3; P(x); y++; P(y);
    z = z - 1; P(z); z *= z; P(z); w++; P(w);
    p->a++; p->b += 2047; p->c += 2; P(s.a); P(s.b); P(s.c);
    (*q)++; q[1]--; ++q[2]; P(garr[0]); P(garr[1]); P(garr[2]); puts("");
    /* the types of results */
    printf("%d %d %d %d %d\n", (int)sizeof(x + 1), (int)sizeof(x + 1uwb),
           (int)sizeof(-1wb), (int)sizeof(255uwb), (int)sizeof(256wb));
    printf("%d %d %d %d %d\n", (int)sizeof(_BitInt(8)), (int)sizeof(_BitInt(9)),
           (int)sizeof(_BitInt(17)), (int)sizeof(_BitInt(33)),
           (int)sizeof(unsigned _BitInt(64)));
    P(x + 1); P(x + 1wb); P(x + 1uwb); P(~x); P(-x); P(0uwb - 1); P(K + 1uwb); puts("");
    P(addu(4000, 100)); P(adds(2047, 1)); P(ret(513)); puts("");
    /* usual arithmetic conversions against standard types */
    { unsigned _BitInt(63) a = 5; long long b = -7; P(a + b); printf("%d ", a + b < 0); }
    { unsigned _BitInt(31) a = 5; int b = -7; printf("%d ", a + b < 0); }
    { unsigned _BitInt(40) a = 5; int b = -7; P(a + b); }
    printf("%d\n", (unsigned _BitInt(40))0xffffffffff == -1);
    /* conversions */
    { unsigned _BitInt(3) a = 5; x = a; P(x); }
    { _BitInt(3) a = -3; x = a; P(x); }
    { double d = 11.0; x = d; P(x); }
    { unsigned _BitInt(7) t = 1; t <<= 6; P(t); t <<= 1; P(t); }
    { _BitInt(6) t = -32; t = -t; P(t); }
    { unsigned _BitInt(12) t = 4095; t = t * t; P(t); }
    { _Bool bb = (u4)16; printf("%d ", bb); }
    switch ((unsigned _BitInt(3))11) { case 3: puts("three"); break; default: puts("other"); }
    /* _Generic tells _BitInt types apart */
    printf("%d %d %d\n", _Generic(x, u4: 1, unsigned char: 2, default: 0),
           _Generic((_BitInt(8))0, unsigned _BitInt(8): 1, _BitInt(8): 2, default: 3),
           _Generic(1uwb + 1uwb, unsigned _BitInt(1): 1, default: 0));
    /* local aggregates with non-constant initialisers */
    {   struct S t = { v, vw, ll }, t2 = { .c = ll, .a = 33 };
        u4 a[2] = { v, 9 };
        P(t.a); P(t.b); P(t.c); P(t2.a); P(t2.c); P(a[0]); P(a[1]); puts("");
    }
    return 0;
}

// CHECK: 1
// CHECK: 1 fffffffffffffff9 ffffffffff ffffffff00000000 4 fffffffffffff800 ffffffffff 0 1
// CHECK: 0 f 2 fffffffffffffff8 ffffffffff 1 ffff800000000000 5 ffffffffffffffff 1 0 f 2
// CHECK: 4 1 1 1 2
// CHECK: 1 2 4 8 8
// CHECK: 3 3 3 d e ffffffffffffffff 0
// CHECK: 4 fffffffffffff800 1
// CHECK: fffffffffffffffe 1 1 fffffffffe 1
// CHECK: 5 d b 40 0 ffffffffffffffe0 1 0 three
// CHECK: 1 2 1
// CHECK: 4 fffffffffffff800 ffffffffff 1 ffffffffff 4 9
