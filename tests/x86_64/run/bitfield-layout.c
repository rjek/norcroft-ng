// Bitfield layout as the psABI has it: sharing storage with other members,
// crossing boundaries, zero-width and unnamed fields, 64-bit fields, and
// static initialisers.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
void *memset(void *, int, unsigned long);
static void dump(const char *s, const void *p, int n)
{   const unsigned char *b = p; int i;
    printf("%s", s);
    for (i = 0; i < n; i++) printf(" %02x", b[i]);
    printf("\n");
}
/* Bitfields share storage with preceding members, and move to the next  */
/* boundary of their type only if they would cross it.                   */
struct a { char c; int x : 12; short y : 7; char z : 4; };
struct b { short s; int x : 17; };                  /* x crosses: moves   */
struct c { int i; long long x : 20; long long y : 44; };
struct d { char c; int : 0; char d; };              /* :0 aligns          */
struct e { char c; int : 4; };                      /* unnamed: no align  */
struct f { unsigned char a : 3, b : 6, c : 7; };
struct g { char c[5]; int x : 3; unsigned long long y : 60; };
struct w { char pad; struct a a; struct b b; struct c c; struct d d; struct e e; struct f f; struct g g; };
static struct a ia = { 1, -5, 33, 7 };
static struct c ic = { 3, -1, 0x123456789ALL };
static struct f iff = { 5, 40, 100 };
static struct g ig = { "abcd", -2, 0x0fedcba987654321ULL };
int main(void)
{   struct c c; struct g g;
    printf("%d %d %d %d %d %d %d %d\n", (int)sizeof(struct a), (int)sizeof(struct b),
           (int)sizeof(struct c), (int)sizeof(struct d), (int)sizeof(struct e),
           (int)sizeof(struct f), (int)sizeof(struct g), (int)sizeof(struct w));
    dump("a", &ia, sizeof ia);
    dump("c", &ic, sizeof ic);
    dump("f", &iff, sizeof iff);
    dump("g", &ig, sizeof ig);
    memset(&c, 0, sizeof c); c.y = -1; dump("c.y", &c, sizeof c);
    memset(&g, 0, sizeof g); g.y = 1; g.x = -1; dump("g.x.y", &g, sizeof g);
    printf("%d %d %d %lld %llx\n", ia.x, ia.y, ia.z, (long long)ic.y, (unsigned long long)ig.y);
    ic.x += 7; ig.y >>= 4;
    printf("%lld %llx %d\n", (long long)ic.x, (unsigned long long)ig.y, iff.b + iff.c);
    return 0;
}

// CHECK: 4 8 16 5 2 3 16 64
// CHECK: a 01 fb 1f 3a
// CHECK: c 03 00 00 00 ff ff 0f 00 9a 78 56 34 12 00 00 00
// CHECK: f 05 28 64
// CHECK: g 61 62 63 64 00 06 00 00 21 43 65 87 a9 cb ed 0f
// CHECK: c.y 00 00 00 00 00 00 00 00 ff ff ff ff ff 0f 00 00
// CHECK: g.x.y 00 00 00 00 00 07 00 00 01 00 00 00 00 00 00 00
// CHECK: -5 33 7 78187493530 fedcba987654321
// CHECK: 6 fedcba98765432 140
