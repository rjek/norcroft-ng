// C99's designated initialisers, compound literals and non-constant
// initialisers of automatic aggregates.
// RUN: %cc -std=c99 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
struct P { int x, y, z; };
struct N { int a[3]; struct P p; char name[8]; double d; };
union U { int i; float f; char c[4]; };
struct A { int k; union { int ui; short us; }; struct { int s1, s2; }; };
static struct P sp = { .z = 3, .x = 1 };
static int sa[6] = { [4] = 9, [1] = 2, 3 };
static int so[] = { [5] = 1, [2] = 7 };
static struct N sn = { .p.y = 7, .a[1] = 5, 6, 7, .name = "hi", .d = 2.5 };
static union U su = { .f = 1.5f };
static struct A sx = { .s2 = 4, .ui = 3, .k = 1 };
static int m2[2][3] = { [1] = { 4, 5 }, [0][2] = 3 };
static const char *strs[] = { [2] = "two", [0] = "zero" };
int *g = (int[]){ 4, 5, 6 };
struct P *gp = &(struct P){ .y = 9 };
int sum(struct P p) { return p.x + p.y; }
int f(int v) { return v * 10; }
int main(int argc, char **argv)
{   int i, t = 0;
    struct P ap = { .y = argc + 1, .x = f(2) };
    struct N an = { .d = 1.25, .p = { .z = argc }, .a = { [2] = f(argc) } };
    int dyn[3] = { argc, f(argc), argc + 2 };
    int *p = (int[]){ 1, 2, 3 };
    struct P *q = &(struct P){ argc, argc * 2 };
    for (i = 0; i < 3; i++) { int *r = (int[]){ i, i * 10 }; t += r[1]; }
    printf("%d %d %d |", sp.x, sp.y, sp.z);
    for (i = 0; i < 6; i++) printf(" %d", sa[i]);
    printf(" | %zu %d %d\n", sizeof so / sizeof so[0], so[2], so[5]);
    printf("%d %d %d %d %d %s %g | %g %d %d %d %d\n", sn.a[0], sn.a[1], sn.a[2],
           sn.p.x, sn.p.y, sn.name, sn.d, su.f, sx.k, sx.ui, sx.s1, sx.s2);
    printf("%d %d %d %d | %s %s | %d %d %d | %d %d %d %d | %d %d %d\n",
           m2[0][0], m2[0][2], m2[1][0], m2[1][1], strs[0], strs[2],
           ap.x, ap.y, ap.z, an.p.z, an.a[2], an.a[0], dyn[1], p[2], q->y, t);
    printf("%d %d %d %zu %d %d %d %s\n", g[1], gp->y, sum((struct P){ 3, 4 }),
           sizeof (int[]){ 1, 2, 3 }, (int[]){ 7, 8 }[1], (struct P){ .x = 5 }.x,
           (int){ 42 }, (char[]){ "str" });
    return 0;
}

// CHECK: 1 0 3 | 0 2 3 0 9 0 | 6 7 1
// CHECK: 0 5 6 7 7 hi 2.5 | 1.5 1 3 0 4
// CHECK: 0 3 4 5 | zero two | 20 2 0 | 1 10 0 10 | 3 2 30
// CHECK: 5 9 7 12 8 5 42 str
