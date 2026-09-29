// <stdarg.h> with int and double arguments.
// RUN: %cc %s -o %t && %t
// REQUIRES: i386, i386-run

#include <stdarg.h>
int printf(const char *, ...);
int isum(int n, ...) { va_list ap; int s = 0; va_start(ap, n); while (n--) s += va_arg(ap, int); va_end(ap); return s; }
double dsum(int n, ...) { va_list ap; double s = 0; va_start(ap, n); while (n--) s += va_arg(ap, double); va_end(ap); return s; }
int main(void) { printf("%d %g\n", isum(4, 1, 2, 3, 4), dsum(3, 1.5, 2.5, -1.0)); return 0; }

// CHECK: 10 3
