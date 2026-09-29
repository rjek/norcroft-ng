// float and double results are returned in the x87 st(0), although the
// arithmetic is done in SSE registers.
// RUN: %cc %s -S -o -
// REQUIRES: i386

double twice(double x) { return x + x; }
float half(float x) { return x / 2.0f; }
double call(double x) { return twice(x) + half((float)x); }

// CHECK: twice:
// CHECK: addsd
// CHECK: fldl
// CHECK: ret
// CHECK: half:
// CHECK: divss
// CHECK: flds
// CHECK: ret
// CHECK: call:
// CHECK: call twice
// CHECK: fstpl
// CHECK: call half
// CHECK: fstps
