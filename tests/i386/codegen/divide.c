// Division uses the fixed edx:eax register pair.
// RUN: %cc %s -S -o -
// REQUIRES: i386

int sdiv(int a, int b) { return a / b; }
unsigned urem(unsigned a, unsigned b) { return a % b; }
int shift(int a, int b) { return a << b; }

// CHECK: sdiv:
// CHECK: cltd
// CHECK: idivl
// CHECK: urem:
// CHECK: xorl %edx, %edx
// CHECK: divl
// CHECK: movl %edx,
// CHECK: shift:
// CHECK: movl
// CHECK: , %ecx
// CHECK: shll %cl,
