// C11's _Generic (with the controlling expression's type after lvalue
// conversion, but not promotion), _Alignof, _Noreturn and <stdalign.h>.
// RUN: %cc -std=c11 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <stdalign.h>
#include <stdnoreturn.h>
#include <stdlib.h>
#define T(x) _Generic((x), int: "int", unsigned: "unsigned", long: "long", \
    long long: "llong", char: "char", signed char: "schar", double: "double", \
    float: "float", char *: "char*", const char *: "cchar*", int *: "int*", \
    void (*)(void): "fnptr", struct S: "S", default: "other")
struct S { int a; };
noreturn void die(void) { exit(0); }
void fn(void) {}
int main(void)
{   const int ci = 1;
    char arr[3];
    struct S s;
    short sh = 1;
    const char *cs = "x";
    printf("%s %s %s %s %s %s %s %s\n", T(1), T(1u), T(1L), T(1LL), T('a'),
           T((signed char)1), T(1.0), T(1.0f));
    printf("%s %s %s %s %s %s %s %s %s\n", T(arr), T(cs), T("lit"), T(&ci),
           T(fn), T(s), T(ci), T(sh), T(sh + 1));
    printf("%zu %zu %zu %zu\n", _Alignof(double), alignof(char),
           _Alignof(struct S), alignof(int[4]));
    die();
}

// CHECK: int unsigned long llong int schar double float
// CHECK: char* cchar* char* other fnptr S int other int
// CHECK: 8 1 4 4
