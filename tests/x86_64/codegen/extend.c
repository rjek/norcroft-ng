// int and long: widening to 64 bits sign or zero extends, and compares
// are done at the operands' width.
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

long sext(int x) { return x; }
unsigned long zext(unsigned x) { return x; }
long lsh(long x, int n) { return x << n; }
int cmp(int a, int b) { return a < b; }
int lcmp(long a, long b) { return a < b; }

// CHECK: sext:

// CHECK: movslq %edi, %rax

// CHECK: zext:

// CHECK: movl %edi, %eax

// CHECK: lsh:

// CHECK: shlq %cl, %rax

// CHECK: cmp:

// CHECK: cmpl %esi, %edi

// CHECK: lcmp:

// CHECK: cmpq %rsi, %rdi
