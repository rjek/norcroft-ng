// Pointers in initialised data (8-byte relocations), function pointers and a switch table, in a PIE.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <string.h>
static int sv = 5;
int gv = 7;
extern char **environ;
static const char *names[] = { "zero", "one", "two", "three" };
int *ptrs[] = { &sv, &gv, &gv + 1, 0 };
int (*fp)(const char *, ...) = printf;
struct { const char *s; long n; void *p; } tab[] = { { "a", 1, &sv }, { "b", -2, (void *)0 } };
static int sq(int x) { return x * x; }
int (*sfp)(int) = sq;
const char *sw(int x) { switch (x) { case 0: return "a"; case 1: return "b"; case 2: return "c"; case 3: return "d"; case 5: return "f"; case 7: return "h"; default: return "?"; } }
int main(void)
{   int i;
    fp("%s %s %d %d %d\n", names[2], tab[0].s, *ptrs[0], *ptrs[1], (int)tab[1].n);
    printf("%d %d %d %s%s%s%s\n", sfp(9), tab[0].p == &sv, environ != 0, sw(0), sw(3), sw(7), sw(4));
    printf("%zu %p\n", strlen(names[3]), (void *)ptrs[3]);
    for (i = 0; i < 4; i++) printf("%s ", names[i]);
    printf("\n");
    return 0;
}

// CHECK: two a 5 7 -2
// CHECK: 81 1 1 adh?
// CHECK: 5 (nil)
// CHECK: zero one two three
