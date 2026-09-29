// A variadic function saves the argument registers in the psABI's
// register save area, and va_start describes it; a variadic call sets al
// to the number of vector registers used.
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

#include <stdarg.h>
int printf(const char *, ...);
double sum(int n, ...)
{   va_list ap; double s = 0;
    va_start(ap, n);
    while (n--) s += va_arg(ap, double);
    va_end(ap);
    return s;
}
void call(void) { printf("%d %g %g\n", 1, 2.0, 3.0); }

// CHECK: sum:

// CHECK: subq $224, %rsp

// CHECK: movq %rdi, -192(%rbp)

// CHECK: movq %r9, -152(%rbp)

// CHECK: movsd %xmm0, -144(%rbp)

// CHECK: movsd %xmm7, -32(%rbp)

// CHECK: movl $8, %eax

// CHECK: movl $48, %eax

// CHECK: leaq 16(%rbp), %rax

// CHECK: leaq -192(%rbp), %rax

// CHECK: call:

// CHECK: movl $2, %eax

// CHECK: call printf@PLT
