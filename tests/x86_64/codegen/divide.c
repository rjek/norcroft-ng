// Division: 32-bit (cltd/idivl, divl) or 64-bit (cqto/idivq, divq)
// according to the operand type.
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

int sdiv(int a, int b) { return a / b; }
unsigned urem(unsigned a, unsigned b) { return a % b; }
long ldiv_(long a, long b) { return a / b; }
unsigned long ulrem(unsigned long a, unsigned long b) { return a % b; }

// CHECK: sdiv:

// CHECK: cltd

// CHECK: idivl %esi

// CHECK: urem:

// CHECK: xorl %edx, %edx

// CHECK: divl %esi

// CHECK: ldiv_:

// CHECK: cqto

// CHECK: idivq %rsi

// CHECK: ulrem:

// CHECK: divq %rsi

// CHECK: movq %rdx,
