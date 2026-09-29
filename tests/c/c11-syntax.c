// The syntax of C11 on a target of its own.
// RUN: %cc -std=c11 %s -c -o %t.o

#include <stdalign.h>
#include <stdnoreturn.h>
struct S { char c; _Alignas(8) int x; union { int i; float f; }; };
_Static_assert(alignof(struct S) == 8, "aligned member");
static _Alignas(16) char buf[4];
static const unsigned short *s16 = u"abc";
static const unsigned int *s32 = U"abc";
static const char *s8 = u8"abc";
noreturn void stop(void);
int f(int x)
{   buf[0] = (char)_Generic(x, int: 1, default: 2);
    return buf[0] + s16[1] + s32[2] + s8[0] + (int)_Alignof(double);
}
