// C23's binary constants, digit separators, u8 characters, and its
// preprocessor: #elifdef, #elifndef, __has_include, __has_c_attribute,
// true and false in #if, __VA_OPT__ and #embed.
// RUN: %cc -std=c23 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#define A
#ifdef NOPE
int x = 0;
#elifdef A
int x = 1;
#endif
#ifdef NOPE
int y = 0;
#elifndef B
int y = 2;
#endif
#if __has_include(<stdio.h>) && !__has_include("nonesuch.h") && defined(__has_include)
int h = 1;
#else
int h = 0;
#endif
#if __has_c_attribute(fallthrough) && !__has_c_attribute(bogus)
int at = 1;
#else
int at = 0;
#endif
#if true && !false
int tf = 1;
#else
int tf = 0;
#endif
#define ID(a) a
#define P(fmt, ...) printf(fmt __VA_OPT__(,) __VA_ARGS__)
#define Q(a, ...) (a __VA_OPT__(+ __VA_ARGS__))
static const unsigned char me[] = {
#embed "c23-lex-pp.c" limit(2) suffix(, 0)
};
int main(void)
{   P("start\n");
    P("%d %d %d %d %d %d %d\n", x, y, h, at, tf, Q(1), Q(1, 2));
    printf("%d %ld %lld %d %x %d %d %zu\n", 0b1010, 1'000'000L, 0x1'0000'0000LL,
           0B11'01, 0xF'F, ID(1'2), u8'a', sizeof(u8'a'));
    printf("%zu %c%c %d\n", sizeof me, me[0], me[1], me[2]);
    return 0;
}

// CHECK: start
// CHECK: 1 2 1 1 1 1 3
// CHECK: 10 1000000 4294967296 13 ff 12 97 1
// CHECK: 3 // 0
