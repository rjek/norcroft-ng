// psABI struct passing and returning, called from code compiled by the host cc.
// RUN: %cc -DIMPL -c %s -o %t.impl.o && cc -c %s -o %t.o && cc %t.o %t.impl.o -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include "abi.c"
// CHECK: ii 5 -5
// CHECK: ll 80740352 -78
// CHECK: dd 7 0.875
// CHECK: ff 3.5 1.5
// CHECK: fff 1.25 3.75 -1.25
// CHECK: id 63 4.5
// CHECK: di 5 36
// CHECK: c3 10 11 12
// CHECK: c9 20 2000 20 24
// CHECK: l3 6 36 216
// CHECK: fi 6 12
// CHECK: d1 2.25
// CHECK: c12 1 16 34
// CHECK: all -3992457900988925524
// CHECK: many 2870
// CHECK: nofit 2584
// CHECK: mem 2967
// CHECK: floats 506
