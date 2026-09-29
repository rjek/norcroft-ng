// va_arg of integers, doubles, pointers and structs, in registers and on the stack; vprintf and va_copy.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdarg.h>
#include <stdio.h>
struct two { long a, b; };
struct big { long a, b, c; };
static void pr(const char *fmt, ...)
{   va_list ap, aq;
    va_start(ap, fmt);
    va_copy(aq, ap);
    vprintf(fmt, ap);
    vprintf(fmt, aq);
    va_end(aq);
    va_end(ap);
}
static double mix(int n, ...)
{   va_list ap; double s = 0; int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) {
        switch (va_arg(ap, int)) {
        case 0: s += va_arg(ap, int); break;
        case 1: s += va_arg(ap, double); break;
        case 2: s += va_arg(ap, long); break;
        case 3: s += *va_arg(ap, char *); break;
        case 4: { struct two t = va_arg(ap, struct two); s += t.a * 10 + t.b; break; }
        case 5: { struct big t = va_arg(ap, struct big); s += t.a * 100 + t.b * 10 + t.c; break; }
        }
    }
    va_end(ap);
    return s;
}
static long named(long a, double b, long c, double d, ...)
{   va_list ap; long s = a + (long)b + c + (long)d;
    va_start(ap, d);
    s += va_arg(ap, long) * 1000;
    s += (long)(va_arg(ap, double) * 100);
    va_end(ap);
    return s;
}
int main(void)
{   struct two t = { 3, 4 };
    struct big b = { 5, 6, 7 };
    pr("%d %s %f %ld %c %g %d %d %d %d %d %d %g %g %g %g %g %g %g\n",
       1, "two", 3.0, 4L, '5', 6.5, 7, 8, 9, 10, 11, 12,
       1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5);
    printf("%g\n", mix(12, 0, 1, 1, 2.5, 2, 1L << 33, 3, "A", 0, 5, 1, 0.25,
                       0, 6, 0, 7, 1, 1.0, 1, 2.0, 1, 3.0, 1, 4.0));
    printf("%g\n", mix(3, 4, t, 5, b, 0, 9));
    printf("%g\n", mix(9, 0, 1, 0, 2, 0, 3, 0, 4, 4, t, 5, b, 1, 1.0, 0, 8, 4, t));
    printf("%ld\n", named(1, 2.0, 3, 4.0, 5L, 6.25));
    return 0;
}

// CHECK: 1 two 3.000000 4 5 6.5 7 8 9 10 11 12 1.5 2.5 3.5 4.5 5.5 6.5 7.5
// CHECK: 1 two 3.000000 4 5 6.5 7 8 9 10 11 12 1.5 2.5 3.5 4.5 5.5 6.5 7.5
// CHECK: 8.58993e+09
// CHECK: 610
// CHECK: 654
// CHECK: 5635
