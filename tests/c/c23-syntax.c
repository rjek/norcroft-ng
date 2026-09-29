// The syntax of C23 on a target of its own.
// RUN: %cc -std=c23 %s -c -o %t.o

#include <stddef.h>
constexpr int N = 4;
enum E : unsigned char { A, B };
struct P { int x; };
struct P { int x; };
[[nodiscard]] static int twice(int x) { return 2 * x; }
int f(int, int b)
{   auto c = b + 0b1'01;
    typeof(c) arr[N] = {};
    bool ok = true;
    int *p = nullptr;
    static_assert(sizeof(enum E) == 1);
    [[maybe_unused]] int unused;
    return twice(c) + arr[0] + ok + (p == nullptr) + (int)alignof(int);
}
