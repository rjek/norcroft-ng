// Unsigned long long arithmetic on the result of another (which is done
// by calling a function that returns long long) stays unsigned.
// RUN: %cc %s -o %t && %t
// REQUIRES: i386, i386-run

int printf(const char *, ...);
unsigned long long shifts(unsigned long long x) { return (x << 16) >> 16; }
unsigned long long widen(int a) { return ((unsigned long long)a << 31) >> 31; }
unsigned long long divide(unsigned long long x) { return (x * 1) / 3; }
int compare(unsigned long long x) { return (x + 0) > 1; }
int main(void)
{   volatile unsigned long long a = 0xffffd4f5f8130c42ull;
    printf("%llx %llx %llx %d\n", shifts(a), widen(-1), divide(a), compare(a));
    return 0;
}

// CHECK: d4f5f8130c42 1ffffffff 555546fca806596b 1
