// C23's keywords (bool, true, false, nullptr, static_assert, alignas,
// alignof, thread_local, typeof, typeof_unqual, constexpr, auto), and
// attributes, which are ignored.
// RUN: %cc -std=c23 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <stddef.h>
constexpr int N = 4;
[[nodiscard]] int f(void) { return 1; }
[[deprecated("x")]] void g(void) {}
struct [[maybe_unused]] S { int a [[maybe_unused]]; };
int h(int, int b) { return b; }
int none() { return 7; }
int main(void)
{   bool b = true;
    int x = 3, a[N];
    typeof(x) y = 4;
    typeof_unqual(const int) z = 5;
    const int ci = 7;
    typeof_unqual(ci) w = 8;
    int *p = nullptr;
    nullptr_t np = nullptr;
    constexpr int M = N + 2;
    int m[M];
    auto d = 3.5;
    auto q = &x;
    [[maybe_unused]] int u;
    static_assert(M == 6);
    static_assert(sizeof(char) == 1, "char");
    alignas(8) static char buf[3];
    thread_local static int tl = 2;
    w++;
    switch (f()) { case N - 3: x++; [[fallthrough]]; default: break; }
    printf("%d %d %d %d %d %zu %zu %d %d\n", b, false, y, z, w, sizeof a / sizeof *a,
           sizeof m / sizeof *m, p == nullptr, np == 0);
    printf("%g %d %zu %zu %d %d %d %d %zu\n", d, *q, alignof(double), sizeof(true),
           h(1, 2), none(), _Generic(true, bool: 1, default: 0), tl, sizeof buf);
    return 0;
}

// CHECK: 1 0 4 5 9 4 6 1 1
// CHECK: 3.5 4 8 1 2 7 1 2 3
