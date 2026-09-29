// C23's enums with fixed underlying types, identical redefinitions of
// structs and unions, compound literals with storage classes, and
// va_start with one argument.
// RUN: %cc -std=c23 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <stdarg.h>
enum E : unsigned char { EA = 1, EB = 255 };
enum F : short { FX = -5, FY };
enum : long long { Z = 7 };
struct P { int x; unsigned f : 3; };
struct P { int x; unsigned f : 3; };
union U { int i; float f; };
union U { int i; float f; };
int *counter(void) { return (static int[]){ 0 }; }
int first(...) { va_list ap; int n; va_start(ap); n = va_arg(ap, int); va_end(ap); return n; }
int main(void)
{   struct Q { char c; };
    struct Q { char c; };
    struct Q q = { 'q' };
    struct P p = { 4, 5 };
    enum E e = EB;
    (*counter())++;
    (*counter())++;
    printf("%zu %zu %zu %d %d %d %c %zu %d %d\n", sizeof(enum E), sizeof(enum F),
           sizeof(Z), e, FY, p.f, q.c, sizeof(union U), *counter(), first(42));
    return 0;
}

// CHECK: 1 2 8 255 -4 5 q 4 2 42
