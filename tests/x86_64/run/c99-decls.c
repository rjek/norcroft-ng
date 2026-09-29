// C99's declarations after statements and in for statements, and C23's
// labels at the end of blocks, with the scopes they have.
// RUN: %cc -std=c23 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
int i = 100;
int main(void)
{   int s = 0;
    printf("%d ", i);
    int i = 5;
    printf("%d ", i);
    for (int i = 0, j = 10; i < 3; i++, j--) { s += i * j; int i = 7; s += i; }
    printf("%d %d ", i, s);
    for (int k = 0; k < 2; k++) for (int k = 0; k < 2; k++) s++;
    printf("%d ", s);
    { printf("%d ", i); int i = 9; printf("%d ", i); }
    switch (i) { case 5: s = 1; int t = 3; s += t; break; default: s = 0; }
    goto end;
end:
    printf("%d %d\n", i, s);
    { if (s) goto out; s = 99; out: }
    return 0;
}

// CHECK: 100 5 5 46 50 5 9 5 4
