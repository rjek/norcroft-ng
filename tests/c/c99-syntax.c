// The syntax of C99 (and of C23's labels) on a target of its own.
// RUN: %cc -std=c99 %s -c -o %t.o

#include <stdbool.h>
struct P { int x, y; };
union U { int i; float f; };
static struct P sp = { .y = 2 };
static int sa[] = { [3] = 1, [1] = 2 };
static union U su = { .f = 1.0f };
int *gp = (int[]){ 1, 2 };
inline int twice(int x) { return 2 * x; }
extern int twice(int);
int f(int n, int a[static 4], int *restrict p)
{   _Bool b = n;
    n += twice(n); // comment
    int m = n + a[0];
    for (int i = 0; i < m; i++) *p += i;
    struct P q = { .x = n, .y = m };
    int *r = (int[]){ n, m };
    return b + q.x + r[1] + (int){ 3 } + sizeof (int[]){ 1, 2, 3 } + (int)__func__[0];
}
