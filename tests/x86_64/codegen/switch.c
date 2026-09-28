// A dense switch uses a position-independent table of offsets.
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

int f(int c)
{   switch (c) {
    case 1: return 10; case 2: return 20; case 3: return 35;
    case 4: return 47; case 5: return 51; case 6: return 69;
    default: return 0;
    }
}

// CHECK: cmpq $7, %rax

// CHECK: jae

// CHECK: leaq .L1_x2(%rip), %r11

// CHECK: movslq 4(%r11,%rax,4), %r10

// CHECK: addq %r11, %r10

// CHECK: jmp *%r10

// CHECK: .section .rodata

// CHECK: .L1_x2:

// CHECK: .long .L1_5-.L1_x2

// CHECK: .text
