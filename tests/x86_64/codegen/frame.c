// The frame: rbp, callee-saved registers pushed (keeping rsp 16-byte
// aligned at calls), and calls to external functions via the PLT.
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

int g(int);
int f(int a, int b)
{   int x = g(a), y = g(b), z = g(x + y);
    return x + y + z;
}

// CHECK: f:

// CHECK: pushq %rbp

// CHECK: movq %rsp, %rbp

// CHECK: pushq %rbx

// CHECK: pushq %r12

// CHECK: subq $64, %rsp

// CHECK: call g@PLT

// CHECK: leaq -16(%rbp), %rsp

// CHECK: popq %r12

// CHECK: popq %rbx

// CHECK: popq %rbp

// CHECK: ret
