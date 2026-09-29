// Structs of up to 16 bytes are returned in rax/rdx and xmm0/xmm1 by
// class (internally in rax and r10); larger ones via a hidden pointer,
// which is also returned in rax.
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

struct two { long a, b; };
struct dd { double x, y; };
struct mix { double d; long l; };
struct big { long a[3]; };
struct two two(long a) { struct two t; t.a = a; t.b = -a; return t; }
struct dd dd(double a) { struct dd t; t.x = a; t.y = a * 2; return t; }
struct mix mix(double a) { struct mix t; t.d = a; t.l = 3; return t; }
struct big big(long a) { struct big t; t.a[0] = t.a[1] = t.a[2] = a; return t; }
long use(void) { struct two t = two(4); struct dd d = dd(1.5); return t.b + (long)d.y; }

// CHECK: two:

// CHECK: movq %r10, %rdx

// CHECK: ret

// CHECK: dd:

// CHECK: movq %rax, %xmm0

// CHECK: movq %r10, %xmm1

// CHECK: mix:

// CHECK: movq %rax, %xmm0

// CHECK: movq %r10, %rax

// CHECK: big:

// CHECK: movq %rdi, -8(%rbp)

// CHECK: movq 16(%rax), %r11
// CHECK: movq %r11, 16(%rdi)

// CHECK: movq -8(%rbp), %rax

// CHECK: use:

// CHECK: call two@PLT

// CHECK: movq %rdx, %r10

// CHECK: call dd@PLT

// CHECK: movq %xmm1, %r10

// CHECK: movq %xmm0, %rax
