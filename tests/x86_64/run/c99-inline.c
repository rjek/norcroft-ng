// C99's inline: an inline definition is also an external one if any
// declaration at file scope is extern or not inline.
// RUN: %cc -std=c99 %s -c -o %t.o && nm %t.o
// REQUIRES: x86_64

inline int a1(int x) { return x + 1; }
inline int a2(int x) { return x + 2; }
extern int a2(int);
extern inline int a3(int x) { return x + 3; }
int a4(int);
inline int a4(int x) { return x + 4; }
inline int a5(int x) { return x + 5; }
int a5(int);
static inline int a6(int x) { return x + 6; }
int use(int x) { return a1(x) + a2(x) + a3(x) + a4(x) + a5(x) + a6(x); }

// CHECK-NO: T a1
// CHECK: T a2
// CHECK: T a3
// CHECK: T a4
// CHECK: T a5
