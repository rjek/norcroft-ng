// The frame keeps esp 16-byte aligned at calls, and saves only the
// callee-save registers that are used.
// RUN: %cc %s -S -o -
// REQUIRES: i386

int g(int);
int f(int n) { return n * g(n - 1); }

// CHECK: f:
// CHECK: pushl %ebp
// CHECK: movl %esp, %ebp
// CHECK: pushl %ebx
// CHECK: subl $20, %esp
// CHECK: call g
// CHECK: popl %ebx
// CHECK: popl %ebp
// CHECK: ret
