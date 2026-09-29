// -I directories are searched before the compiled-in headers.
// RUN: mkdir -p %t.d && echo '#define MINE 42' > %t.d/stdbool.h && %cc -I%t.d %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdbool.h>
#include <limits.h>
int printf(const char *, ...);
int main(void) { printf("%d %d\n", MINE, INT_MAX > 0); return 0; }

// CHECK: 42 1
