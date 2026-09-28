// The i386 ABI leaves the upper bits of eax undefined for char and short
// results, so the caller extends them.
// RUN: %cc %s -S -o -
// REQUIRES: i386

signed char sc(void);
unsigned char uc(void);
short ss(void);
unsigned short us(void);

int f(void) { return sc() + uc() + ss() + us(); }

// CHECK: call sc
// CHECK: movsbl %al,
// CHECK: call uc
// CHECK: andl $255,
// CHECK: call ss
// CHECK: movswl %ax,
// CHECK: call us
// CHECK: andl $65535,
