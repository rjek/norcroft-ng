// A dense switch uses a jump table in .rodata.
// RUN: %cc %s -S -o -
// REQUIRES: i386

int f(int c)
{   switch (c) {
    case 1: return 10; case 2: return 20; case 3: return 35;
    case 4: return 47; case 5: return 51; case 6: return 69;
    default: return 0;
    }
}

// CHECK: cmpl
// CHECK: jae
// CHECK: jmp *.L
// CHECK: .section .rodata
// CHECK: .long .L
// CHECK: .text
