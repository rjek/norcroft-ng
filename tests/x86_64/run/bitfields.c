// Bitfield layout.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
struct a { char c; int d : 5; int e : 3; };
struct b { int x : 3; int y : 30; };
struct c { short s; int f : 17; char t; };
struct d { char c; long long ll : 40; };
int main(void)
{   struct a a; struct c c;
    a.c = 1; a.d = -5; a.e = 3; c.s = 7; c.f = -60000; c.t = 9;
    printf("%d %d %d %d\n", (int)sizeof(struct a), (int)sizeof(struct b),
           (int)sizeof(struct c), (int)sizeof(struct d));
    printf("%d %d %d %d %d %d\n", a.c, a.d, a.e, c.s, c.f, c.t);
    return 0;
}

// CHECK: 4 8 8 8
// CHECK: 1 -5 3 7 -60000 9
