// A struct result is returned via a hidden pointer, which the callee pops
// (ret $4) and also returns in eax; the caller must re-adjust esp.
// RUN: %cc %s -S -o -
// REQUIRES: i386

struct big { int a[4]; };

struct big make(int x)
{   struct big b;
    b.a[0] = b.a[1] = b.a[2] = b.a[3] = x;
    return b;
}

int use(void)
{   struct big b = make(3);
    return b.a[2];
}

// CHECK: make:
// CHECK: movl 8(%ebp), %eax
// CHECK: ret $4
// CHECK: use:
// CHECK: call make
// CHECK: subl $4, %esp
