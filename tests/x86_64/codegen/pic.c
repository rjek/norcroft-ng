// Position-independent code: external data via the GOT, calls via the
// PLT, local data RIP-relative, and 8-byte pointers in data (in
// .data.rel.ro if constant).
// RUN: %cc %s -S -o -
// REQUIRES: x86_64

extern int ext;
extern int ext_fn(int);
static int local;
int *lp = &local;
int *ep = &ext;
const struct { int *p; const char *s; } tab = { &ext, "s" };
int *addr_ext(void) { return &ext; }
int *addr_local(void) { return &local; }
int call(void) { return ext_fn(1) + ext; }

// CHECK: addr_ext:

// CHECK: movq ext@GOTPCREL(%rip), %rax

// CHECK: addr_local:

// CHECK: leaq .Ldata(%rip), %rax

// CHECK: call ext_fn@PLT

// CHECK: movq ext@GOTPCREL(%rip)

// CHECK: .section .data.rel.ro,"aw"

// CHECK: tab:

// CHECK: .quad ext

// CHECK: .data

// CHECK: lp:

// CHECK: .quad .Ldata

// CHECK: ep:

// CHECK: .quad ext
