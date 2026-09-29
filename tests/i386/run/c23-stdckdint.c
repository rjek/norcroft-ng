// C23's <stdckdint.h>: checked addition, subtraction and multiplication
// for various result and operand types, including _BitInt.
// RUN: %cc -std=c23 %s -o %t && %t
// REQUIRES: i386, i386-run

#include <stdio.h>
#include <stdckdint.h>
#include <limits.h>
int main(void)
{   int i; unsigned u; long long ll; unsigned long long ull;
    signed char sc; unsigned char uc; short sh;
    unsigned _BitInt(5) b5; _BitInt(40) b40;
    int n = 0;
    printf("%d ", ckd_add(&i, INT_MAX, 1)); printf("%d\n", i);
    printf("%d ", ckd_add(&i, INT_MAX - 1, 1)); printf("%d\n", i);
    printf("%d ", ckd_sub(&i, INT_MIN, 1)); printf("%d\n", i);
    printf("%d ", ckd_mul(&i, 65536, 65536)); printf("%d\n", i);
    printf("%d ", ckd_mul(&i, -46341, 46340)); printf("%d\n", i);
    printf("%d ", ckd_sub(&u, 0, 1)); printf("%u\n", u);
    printf("%d ", ckd_add(&u, -1, 2)); printf("%u\n", u);
    printf("%d ", ckd_mul(&ll, LLONG_MIN, -1)); printf("%llx\n", (unsigned long long)ll);
    printf("%d ", ckd_mul(&ll, LLONG_MIN, 1)); printf("%llx\n", (unsigned long long)ll);
    printf("%d ", ckd_add(&ull, ULLONG_MAX, ULLONG_MAX)); printf("%llx\n", ull);
    printf("%d ", ckd_mul(&ull, ULLONG_MAX, ULLONG_MAX)); printf("%llx\n", ull);
    printf("%d ", ckd_mul(&ull, 0xffffffffull, 0x100000001ull)); printf("%llx\n", ull);
    printf("%d ", ckd_sub(&ll, 0ull, 0x8000000000000000ull)); printf("%llx\n", (unsigned long long)ll);
    printf("%d ", ckd_add(&sc, 100, 27)); printf("%d\n", sc);
    printf("%d ", ckd_add(&sc, 100, 28)); printf("%d\n", sc);
    printf("%d ", ckd_mul(&uc, 16, 16)); printf("%d\n", uc);
    printf("%d ", ckd_sub(&sh, -32768, 0)); printf("%d\n", sh);
    printf("%d ", ckd_add(&b5, 30, 1)); printf("%d\n", (int)b5);
    printf("%d ", ckd_add(&b5, 30, 2)); printf("%d\n", (int)b5);
    printf("%d ", ckd_mul(&b40, 1ll << 20, 1 << 19)); printf("%lld\n", (long long)b40);
    printf("%d ", ckd_mul(&b40, 1ll << 20, -(1 << 19))); printf("%lld\n", (long long)b40);
    printf("%d ", ckd_mul(&b40, 1ll << 20, 1 << 20)); printf("%lld\n", (long long)b40);
    /* each argument is evaluated once */
    {   int arr[2] = { 0, 0 }, *pp = arr;
        ckd_add(pp++, n++, 10); printf("%d %d %d\n", arr[0], n, (int)(pp - arr));
    }
    return 0;
}

// CHECK: 1 -2147483648
// CHECK: 0 2147483647
// CHECK: 1 2147483647
// CHECK: 1 0
// CHECK: 0 -2147441940
// CHECK: 1 4294967295
// CHECK: 0 1
// CHECK: 1 8000000000000000
// CHECK: 0 8000000000000000
// CHECK: 1 fffffffffffffffe
// CHECK: 1 1
// CHECK: 0 ffffffffffffffff
// CHECK: 0 8000000000000000
// CHECK: 0 127
// CHECK: 1 -128
// CHECK: 1 0
// CHECK: 0 -32768
// CHECK: 0 31
// CHECK: 1 0
// CHECK: 1 -549755813888
// CHECK: 0 -549755813888
// CHECK: 1 0
// CHECK: 10 1 1
