// _Thread_local isn't supported on ARM.
// RUN: %cc -std=c11 %s -c -o %t.o
// EXPECT-ERROR
// CHECK-ERR: _Thread_local is not supported on this target

_Thread_local int t;
