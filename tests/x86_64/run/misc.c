// Globals, statics, strings, function pointers and loops.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
int counter;
static int hidden = 42;
int table[5] = { 1, 2, 3, 4, 5 };
char msg[] = "hello";
const char *msgs[] = { "a", "bb", "ccc" };
int apply(int (*f)(int), int x) { return f(x); }
int twice(int x) { return 2 * x; }
int sq(int x) { return x * x; }
int strlen_(const char *s) { const char *p = s; while (*p) p++; return p - s; }
void bump(int *p) { ++*p; counter += *p; }
int fib(int n) { int a = 0, b = 1; while (n-- > 0) { int t = a + b; a = b; b = t; } return a; }
int main(void)
{   int i, local = 0;
    for (i = 0; i < 5; i++) bump(&table[i]);
    printf("%d %d %d\n", counter, table[4], hidden);
    printf("%s %d %s %d\n", msg, strlen_(msg), msgs[2], strlen_(msgs[1]));
    printf("%d %d\n", apply(twice, 21), apply(sq, 12));
    bump(&local); bump(&local);
    printf("%d %d %d\n", local, fib(30), counter);
    return 0;
}

// CHECK: 20 6 42
// CHECK: hello 5 ccc 2
// CHECK: 42 144
// CHECK: 2 832040 23
