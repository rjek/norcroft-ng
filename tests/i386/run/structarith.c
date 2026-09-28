// Arithmetic on struct members, and returning structs from functions.
// RUN: %cc %s -o %t && %t
// REQUIRES: i386, i386-run

int printf(const char *, ...);
typedef struct { unsigned lo, hi; } ll;
static ll mk(unsigned lo, unsigned hi) { ll r; r.lo = lo, r.hi = hi; return r; }
static ll mul32(unsigned a, unsigned b)
{   unsigned al = a & 0xffff, ah = a >> 16, bl = b & 0xffff, bh = b >> 16;
    unsigned ll_ = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    unsigned mid = (ll_ >> 16) + (lh & 0xffff) + (hl & 0xffff);
    return mk((ll_ & 0xffff) | (mid << 16), hh + (lh >> 16) + (hl >> 16) + (mid >> 16));
}
static ll mul(ll a, ll b) { ll r = mul32(a.lo, b.lo); r.hi += a.lo * b.hi + a.hi * b.lo; return r; }
int main(void)
{   unsigned v[] = { 0, 1, 2, 0xffffffffu, 0x80000000u, 12345, 0xdeadbeefu };
    int i, j;
    for (i = 0; i < 7; i++) for (j = 0; j < 7; j++) {
        ll a = mk(v[i], v[(i + 3) % 7]), b = mk(v[j], v[(j + 5) % 7]);
        ll r = mul(a, b), p = mul32(v[i], v[j]);
        printf("%08x%08x %08x%08x\n", r.hi, r.lo, p.hi, p.lo);
    }
    return 0;
}

// CHECK: 0000000000000000 0000000000000000
// CHECK: ffffffff00000000 0000000000000000
// CHECK: fffffffe00000000 0000000000000000
// CHECK: 0000000100000000 0000000000000000
// CHECK: 8000000000000000 0000000000000000
// CHECK: ffffcfc700000000 0000000000000000
// CHECK: 2152411100000000 0000000000000000
// CHECK: 0000303900000000 0000000000000000
// CHECK: 5eadbeef00000001 0000000000000001
// CHECK: 0000000000000002 0000000000000002
// CHECK: 80000001ffffffff 00000000ffffffff
// CHECK: 0000000280000000 0000000080000000
// CHECK: 7fffffff00003039 0000000000003039
// CHECK: 00000000deadbeef 00000000deadbeef
// CHECK: 0000607200000000 0000000000000000
// CHECK: bd5bae1700000002 0000000000000002
// CHECK: 0000607200000004 0000000000000004
// CHECK: ffffcfcafffffffe 00000001fffffffe
// CHECK: 8000000500000000 0000000100000000
// CHECK: 09156caf00006072 0000000000006072
// CHECK: 287c5338bd5b7dde 00000001bd5b7dde
// CHECK: ffffcfc700000000 0000000000000000
// CHECK: 00000000ffffffff 00000000ffffffff
// CHECK: bd5b7ddffffffffe 00000001fffffffe
// CHECK: 2152410e00000001 fffffffe00000001
// CHECK: fffffffd80000000 7fffffff80000000
// CHECK: 287c8370ffffcfc7 00003038ffffcfc7
// CHECK: 801b620f21524111 deadbeee21524111
// CHECK: 8000000000000000 0000000000000000
// CHECK: 8000000080000000 0000000080000000
// CHECK: 0000000100000000 0000000100000000
// CHECK: ffffffff80000000 7fffffff80000000
// CHECK: 4000000000000000 4000000000000000
// CHECK: 8000181c80000000 0000181c80000000
// CHECK: 6f56df7780000000 6f56df7780000000
// CHECK: 09156cb100000000 0000000000000000
// CHECK: 287c533800003039 0000000000003039
// CHECK: 0000000200006072 0000000000006072
// CHECK: 00006070ffffcfc7 00003038ffffcfc7
// CHECK: 8000788e80000000 0000181c80000000
// CHECK: 0000000009156cb1 0000000009156cb1
// CHECK: 5eade8e1287c5337 000029f2287c5337
// CHECK: 287c533700000000 0000000000000000
// CHECK: 216da323deadbeef 00000000deadbeef
// CHECK: 00000005bd5b7dde 00000001bd5b7dde
// CHECK: bd5b7ddb21524111 deadbeee21524111
// CHECK: 2cb25d5580000000 6f56df7780000000
// CHECK: 2152cb75287c5337 000029f2287c5337
// CHECK: ff0d4af0216da321 c1b1cd12216da321
