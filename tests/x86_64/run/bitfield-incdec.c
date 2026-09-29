// Increment and decrement of long long bitfields, including via a pointer
// found by a function call (so a pointer temporary is needed).
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
struct s { int i; long long f : 40; unsigned long long g : 12; long long h : 3; } x = { 1, 5, 4095, 1 }, a[2];
struct s *next(void) { static int n; return &a[n++ & 1]; }
int main(void)
{   long long r1 = x.f++, r2 = x.f--, r3 = x.g++, r4 = x.h--, r5 = x.h--;
    struct s *p = &x;
    long long r6 = p->f++;
    a[0].f = 100; a[1].f = 200;
    { long long r7 = next()->f++, r8 = next()->f--;
      printf("%lld %lld %lld %lld %lld %lld %lld %lld\n", r1, r2, r3, r4, r5, r6, r7, r8); }
    x.f--; ++x.g; --x.h;
    printf("%lld %lld %lld %lld %lld %d\n", (long long)x.f, (long long)x.g, (long long)x.h, (long long)a[0].f, (long long)a[1].f, x.i);
    return 0;
}

// CHECK: 5 6 4095 1 0 5 100 200
// CHECK: 5 1 -2 101 199 1
