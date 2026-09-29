// C99's preprocessor: variadic macros (and gcc's , ## __VA_ARGS__),
// _Pragma, hexadecimal floating constants, universal character names
// (and UTF-8 in wide strings), and __STDC_VERSION__ and __STDC_HOSTED__.
// RUN: %cc -std=c99 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <wchar.h>
#define P(fmt, ...) printf(fmt, __VA_ARGS__)
#define Q(...) printf(__VA_ARGS__)
#define S(...) #__VA_ARGS__
#define N(x, rest...) printf(x, rest)
#define L(fmt, ...) printf("[" fmt "]", ## __VA_ARGS__)
#define DO(x) _Pragma(#x)
DO(STDC FP_CONTRACT OFF)
_Pragma("STDC FENV_ACCESS OFF")
int main(void)
{   const char *u = "\u00e9\u20ac\U0001F600";
    const wchar_t *w = L"\u00e9" "x", *w8 = L"é€";
    int i;
    P("%d %d ", 1, (2, 3)); Q("x "); N("%d %s ", 5, "six");
    L("a"); L("%d", 2);
    printf(" %s|%s\n", S(a, b,  c), S());
    printf("%a %a %a %g\n", 0x1.8p1, 0x10p-2f, 0x.8p0, 0x1p-1074);
    for (i = 0; u[i]; i++) printf("%02x", (unsigned char)u[i]);
    printf(" %x %zu %x %x %ld %d\n", (unsigned)w[0], wcslen(w), (unsigned)w8[0], (unsigned)w8[1], __STDC_VERSION__, __STDC_HOSTED__);
    return 0;
}

// CHECK: 1 3 x 5 six [a][2] a, b, c|
// CHECK: 0x1.8p+1 0x1p+2 0x1p-1 4.94066e-324
// CHECK: c3a9e282acf09f9880 e9 2 e9 20ac 199901 1
