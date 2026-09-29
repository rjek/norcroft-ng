// psABI struct passing and returning, calling code compiled by the host cc.
// RUN: cc -DIMPL -c %s -o %t.impl.o && %cc -c %s -o %t.o && cc %t.o %t.impl.o -o %t && %t
// REQUIRES: x86_64, x86_64-run
// (abi-reverse.c tests the other direction.)

struct ii { int a, b; };
struct ll { long a, b; };
struct dd { double a, b; };
struct ff { float a, b; };
struct fff { float a, b, c; };
struct id { int a; double b; };
struct di { double a; int b; };
struct c3 { char c[3]; };
struct c9 { char a; short b; char c[5]; };
struct l3 { long a, b, c; };
struct fi { float a; int b; };
struct d1 { double a; };
struct c12 { char c[12]; };
struct uf { union { float f; int i; } u; float g; };

struct ii r_ii(int x);
struct ll r_ll(long x);
struct dd r_dd(double x);
struct ff r_ff(float x);
struct fff r_fff(float x);
struct id r_id(int x);
struct di r_di(double x);
struct c3 r_c3(int x);
struct c9 r_c9(int x);
struct l3 r_l3(long x);
struct fi r_fi(int x);
struct d1 r_d1(double x);
struct c12 r_c12(int x);

long a_all(struct ii, struct ll, struct dd, struct ff, struct fff, struct id,
           struct di, struct c3, struct c9, struct l3, struct fi, struct d1, struct c12);
double a_many(int a, double b, int c, double d, int e, double f, int g, double h,
              int i, double j, int k, double l, int m, double n, int o, double p,
              int q, double r, float s, int t);
long a_nofit(long a, long b, long c, long d, long e, struct ll f, long g, struct dd h,
             double i, double j, double k, double l, double m, double n, struct dd o, double p);
long a_mem(struct l3 a, int b, struct c12 c, int d, struct l3 e);
double a_floats(float a, float b, float c, float d, float e, float f, float g,
                float h, float i, float j, float k);

#ifdef IMPL

struct ii r_ii(int x) { struct ii r; r.a = x; r.b = -x; return r; }
struct ll r_ll(long x) { struct ll r; r.a = x << 20; r.b = ~x; return r; }
struct dd r_dd(double x) { struct dd r; r.a = x * 2; r.b = x / 4; return r; }
struct ff r_ff(float x) { struct ff r; r.a = x + 1; r.b = x - 1; return r; }
struct fff r_fff(float x) { struct fff r; r.a = x; r.b = x * 3; r.c = -x; return r; }
struct id r_id(int x) { struct id r; r.a = x * 7; r.b = x * 0.5; return r; }
struct di r_di(double x) { struct di r; r.a = x + 0.25; r.b = (int)x * 9; return r; }
struct c3 r_c3(int x) { struct c3 r; r.c[0] = x; r.c[1] = x + 1; r.c[2] = x + 2; return r; }
struct c9 r_c9(int x) { struct c9 r; int i; r.a = x; r.b = x * 100; for (i = 0; i < 5; i++) r.c[i] = x + i; return r; }
struct l3 r_l3(long x) { struct l3 r; r.a = x; r.b = x * x; r.c = x * x * x; return r; }
struct fi r_fi(int x) { struct fi r; r.a = x * 1.5f; r.b = x * 3; return r; }
struct d1 r_d1(double x) { struct d1 r; r.a = x * x; return r; }
struct c12 r_c12(int x) { struct c12 r; int i; for (i = 0; i < 12; i++) r.c[i] = x + i * 3; return r; }

long a_all(struct ii a, struct ll b, struct dd c, struct ff d, struct fff e, struct id f,
           struct di g, struct c3 h, struct c9 i, struct l3 j, struct fi k, struct d1 l, struct c12 m)
{   long s = a.a * 3 + a.b;
    s = s * 31 + b.a + b.b;
    s = s * 31 + (long)(c.a * 8 + c.b);
    s = s * 31 + (long)(d.a * 4 + d.b);
    s = s * 31 + (long)(e.a + e.b * 2 + e.c * 5);
    s = s * 31 + f.a + (long)(f.b * 16);
    s = s * 31 + (long)(g.a * 4) + g.b;
    s = s * 31 + h.c[0] + h.c[1] * 2 + h.c[2] * 3;
    s = s * 31 + i.a + i.b + i.c[0] + i.c[4] * 7;
    s = s * 31 + j.a + j.b * 3 + j.c * 5;
    s = s * 31 + (long)(k.a * 2) + k.b;
    s = s * 31 + (long)(l.a * 8);
    s = s * 31 + m.c[0] + m.c[11] * 3 + m.c[8];
    return s;
}
double a_many(int a, double b, int c, double d, int e, double f, int g, double h,
              int i, double j, int k, double l, int m, double n, int o, double p,
              int q, double r, float s, int t)
{   return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8 + i * 9 + j * 10 +
           k * 11 + l * 12 + m * 13 + n * 14 + o * 15 + p * 16 + q * 17 + r * 18 + s * 19 + t * 20;
}
long a_nofit(long a, long b, long c, long d, long e, struct ll f, long g, struct dd h,
             double i, double j, double k, double l, double m, double n, struct dd o, double p)
{   return a + b * 2 + c * 3 + d * 4 + e * 5 + f.a * 6 + f.b * 7 + g * 8 +
           (long)(h.a * 9 + h.b * 10 + i * 11 + j * 12 + k * 13 + l * 14 + m * 15 + n * 16 +
                  o.a * 17 + o.b * 18 + p * 19);
}
long a_mem(struct l3 a, int b, struct c12 c, int d, struct l3 e)
{   return a.a + a.b * 2 + a.c * 3 + b * 4 + c.c[0] * 5 + c.c[11] * 6 + d * 7 + e.a * 8 + e.c * 9;
}
double a_floats(float a, float b, float c, float d, float e, float f, float g,
                float h, float i, float j, float k)
{   return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6 + g * 7 + h * 8 + i * 9 + j * 10 + k * 11;
}
#else
#include <stdio.h>
int main(void)
{   struct ii a = r_ii(5); struct ll b = r_ll(77); struct dd c = r_dd(3.5);
    struct ff d = r_ff(2.5f); struct fff e = r_fff(1.25f); struct id f = r_id(9);
    struct di g = r_di(4.75); struct c3 h = r_c3(10); struct c9 i = r_c9(20);
    struct l3 j = r_l3(6); struct fi k = r_fi(4); struct d1 l = r_d1(1.5);
    struct c12 m = r_c12(1);
    struct ll q; struct dd dd2;
    printf("ii %d %d\n", a.a, a.b);
    printf("ll %ld %ld\n", b.a, b.b);
    printf("dd %g %g\n", c.a, c.b);
    printf("ff %g %g\n", d.a, d.b);
    printf("fff %g %g %g\n", e.a, e.b, e.c);
    printf("id %d %g\n", f.a, f.b);
    printf("di %g %d\n", g.a, g.b);
    printf("c3 %d %d %d\n", h.c[0], h.c[1], h.c[2]);
    printf("c9 %d %d %d %d\n", i.a, i.b, i.c[0], i.c[4]);
    printf("l3 %ld %ld %ld\n", j.a, j.b, j.c);
    printf("fi %g %d\n", k.a, k.b);
    printf("d1 %g\n", l.a);
    printf("c12 %d %d %d\n", m.c[0], m.c[5], m.c[11]);
    printf("all %ld\n", a_all(a, b, c, d, e, f, g, h, i, j, k, l, m));
    printf("many %g\n", a_many(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20));
    q.a = 100; q.b = 200; dd2.a = 0.5; dd2.b = 0.25;
    printf("nofit %ld\n", a_nofit(1, 2, 3, 4, 5, q, 7, dd2, 1, 2, 3, 4, 5, 6, dd2, 8));
    printf("mem %ld\n", a_mem(j, 3, m, 4, j));
    printf("floats %g\n", a_floats(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11));
    return 0;
}
#endif

// CHECK: ii 5 -5
// CHECK: ll 80740352 -78
// CHECK: dd 7 0.875
// CHECK: ff 3.5 1.5
// CHECK: fff 1.25 3.75 -1.25
// CHECK: id 63 4.5
// CHECK: di 5 36
// CHECK: c3 10 11 12
// CHECK: c9 20 2000 20 24
// CHECK: l3 6 36 216
// CHECK: fi 6 12
// CHECK: d1 2.25
// CHECK: c12 1 16 34
// CHECK: all -3992457900988925524
// CHECK: many 2870
// CHECK: nofit 2584
// CHECK: mem 2967
// CHECK: floats 506
