// C11's _Alignas, of struct members and of static objects (initialised,
// zero, common and in functions), which may be more aligned than 16.
// RUN: %cc -std=c11 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <stdalign.h>
#include <stddef.h>
struct S { char c; _Alignas(16) int x; char d; };
struct T { char c; alignas(double) char e; };
_Alignas(64) static char buf[10] = { 1 };
alignas(32) int g32;
static _Alignas(128) char bss_buf[3];
int main(void)
{   _Alignas(8) char local[5];
    static _Alignas(256) int lstat = 5;
    printf("%zu %zu %zu %zu %zu\n", sizeof(struct S), alignof(struct S),
           offsetof(struct S, x), offsetof(struct S, d), offsetof(struct T, e));
    printf("%d %d %d %d %d\n", (int)((unsigned long)buf % 64),
           (int)((unsigned long)&g32 % 32), (int)((unsigned long)bss_buf % 128),
           (int)((unsigned long)&lstat % 256), (int)((unsigned long)local % 8));
    return buf[0] + bss_buf[0] + lstat - 6;
}

// CHECK: 32 16 16 20 8
// CHECK: 0 0 0 0 0
