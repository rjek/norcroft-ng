/*
 * runtime/i386/llong.c -- long long support for Norcroft C on i386.
 * SPDX-Licence-Identifier: Apache-2.0
 *
 * The compiler turns long long operations into calls of these functions
 * (see sim.lladd etc. in ncc/mip/builtin.c).  long long arguments are
 * passed by value on the stack, low word first, and long long results
 * come back in edx:eax -- the same as gcc's long long -- because the
 * compiler returns __value_in_regs two-word structs in internal
 * registers 0 and 1, which are eax and edx on i386.
 *
 * This file uses only 32-bit arithmetic, so that it can be compiled by
 * Norcroft C itself.
 */

typedef struct { unsigned lo, hi; } ll;

#define LL __value_in_regs ll

static ll mk(unsigned lo, unsigned hi)
{   ll r;
    r.lo = lo, r.hi = hi;
    return r;
}

static int isneg(ll a) { return (a.hi & 0x80000000u) != 0; }

static ll neg(ll a)
{   return mk(-a.lo, ~a.hi + (a.lo == 0));
}

static ll add(ll a, ll b)
{   unsigned lo = a.lo + b.lo;
    return mk(lo, a.hi + b.hi + (lo < a.lo));
}

static ll sub(ll a, ll b)
{   return mk(a.lo - b.lo, a.hi - b.hi - (a.lo < b.lo));
}

static int ucmp(ll a, ll b)
{   if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
    if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
    return 0;
}

static int scmp(ll a, ll b)
{   if (a.hi != b.hi) return (int)a.hi < (int)b.hi ? -1 : 1;
    if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
    return 0;
}

static ll shl(ll a, unsigned n)
{   if (n >= 64) return mk(0, 0);
    if (n >= 32) return mk(0, a.lo << (n - 32));
    if (n == 0) return a;
    return mk(a.lo << n, (a.hi << n) | (a.lo >> (32 - n)));
}

static ll ushr(ll a, unsigned n)
{   if (n >= 64) return mk(0, 0);
    if (n >= 32) return mk(a.hi >> (n - 32), 0);
    if (n == 0) return a;
    return mk((a.lo >> n) | (a.hi << (32 - n)), a.hi >> n);
}

static ll sshr(ll a, unsigned n)
{   unsigned sign = isneg(a) ? ~0u : 0;
    if (n >= 64) return mk(sign, sign);
    if (n >= 32) return mk((unsigned)((int)a.hi >> (n - 32)), sign);
    if (n == 0) return a;
    return mk((a.lo >> n) | (a.hi << (32 - n)), (unsigned)((int)a.hi >> n));
}

/* The full 64-bit product of two 32-bit numbers.                       */
static ll mul32(unsigned a, unsigned b)
{   unsigned al = a & 0xffff, ah = a >> 16, bl = b & 0xffff, bh = b >> 16;
    unsigned ll_ = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    unsigned mid = (ll_ >> 16) + (lh & 0xffff) + (hl & 0xffff);
    return mk((ll_ & 0xffff) | (mid << 16),
              hh + (lh >> 16) + (hl >> 16) + (mid >> 16));
}

static ll mul(ll a, ll b)
{   ll r = mul32(a.lo, b.lo);
    r.hi += a.lo * b.hi + a.hi * b.lo;
    return r;
}

/* Unsigned division by shift and subtract; *rem gets the remainder.    */
static ll udivmod(ll n, ll d, ll *rem)
{   ll q, r;
    int i;
    q = mk(0, 0), r = mk(0, 0);
    for (i = 63; i >= 0; i--)
    {   r = shl(r, 1);
        r.lo |= (i >= 32 ? n.hi >> (i - 32) : n.lo >> i) & 1;
        if (ucmp(r, d) >= 0)
        {   r = sub(r, d);
            if (i >= 32) q.hi |= 1u << (i - 32); else q.lo |= 1u << i;
        }
    }
    *rem = r;
    return q;
}

static ll sdiv(ll n, ll d)
{   ll r, q;
    int negq = isneg(n) != isneg(d);
    q = udivmod(isneg(n) ? neg(n) : n, isneg(d) ? neg(d) : d, &r);
    return negq ? neg(q) : q;
}

static ll srem(ll n, ll d)
{   ll r;
    (void)udivmod(isneg(n) ? neg(n) : n, isneg(d) ? neg(d) : d, &r);
    return isneg(n) ? neg(r) : r;
}

static ll udiv(ll n, ll d) { ll r; return udivmod(n, d, &r); }
static ll urem(ll n, ll d) { ll r; (void)udivmod(n, d, &r); return r; }

#define TWO32 4294967296.0

static ll ufromd(double d)
{   unsigned hi;
    if (d < 1.0) return mk(0, 0);
    hi = (unsigned)(d / TWO32);
    return mk((unsigned)(d - (double)hi * TWO32), hi);
}

static ll sfromd(double d)
{   return d < 0 ? neg(ufromd(-d)) : ufromd(d);
}

static double utod(ll a)
{   return (double)a.hi * TWO32 + (double)a.lo;
}

static double stod(ll a)
{   return isneg(a) ? -utod(neg(a)) : utod(a);
}

/* double and float results are returned in integer registers.         */
typedef union { double d; ll l; } dbits;
typedef union { float f; unsigned u; } fbits;

static LL dresult(double d)  { dbits x; x.d = d; return x.l; }
static unsigned fresult(float f) { fbits x; x.f = f; return x.u; }

/* The entry points.  "r" forms are reversed: _ll_rsb(a, b) = b - a.    */
LL _ll_not(ll a)                 { return mk(~a.lo, ~a.hi); }
LL _ll_neg(ll a)                 { return neg(a); }
LL _ll_add(ll a, ll b)           { return add(a, b); }
LL _ll_sub(ll a, ll b)           { return sub(a, b); }
LL _ll_rsb(ll a, ll b)           { return sub(b, a); }
LL _ll_mul(ll a, ll b)           { return mul(a, b); }
LL _ll_udiv(ll a, ll b)          { return udiv(a, b); }
LL _ll_urdv(ll a, ll b)          { return udiv(b, a); }
LL _ll_sdiv(ll a, ll b)          { return sdiv(a, b); }
LL _ll_srdv(ll a, ll b)          { return sdiv(b, a); }
LL _ll_urem(ll a, ll b)          { return urem(a, b); }
LL _ll_urrem(ll a, ll b)         { return urem(b, a); }
LL _ll_srem(ll a, ll b)          { return srem(a, b); }
LL _ll_srrem(ll a, ll b)         { return srem(b, a); }
LL _ll_and(ll a, ll b)           { return mk(a.lo & b.lo, a.hi & b.hi); }
LL _ll_or(ll a, ll b)            { return mk(a.lo | b.lo, a.hi | b.hi); }
LL _ll_eor(ll a, ll b)           { return mk(a.lo ^ b.lo, a.hi ^ b.hi); }
LL _ll_shift_l(ll a, unsigned n) { return shl(a, n); }
LL _ll_ushift_r(ll a, unsigned n){ return ushr(a, n); }
LL _ll_sshift_r(ll a, unsigned n){ return sshr(a, n); }

int _ll_cmpeq(ll a, ll b)        { return ucmp(a, b) == 0; }
int _ll_cmpne(ll a, ll b)        { return ucmp(a, b) != 0; }
int _ll_ucmpgt(ll a, ll b)       { return ucmp(a, b) > 0; }
int _ll_ucmpge(ll a, ll b)       { return ucmp(a, b) >= 0; }
int _ll_ucmplt(ll a, ll b)       { return ucmp(a, b) < 0; }
int _ll_ucmple(ll a, ll b)       { return ucmp(a, b) <= 0; }
int _ll_scmpgt(ll a, ll b)       { return scmp(a, b) > 0; }
int _ll_scmpge(ll a, ll b)       { return scmp(a, b) >= 0; }
int _ll_scmplt(ll a, ll b)       { return scmp(a, b) < 0; }
int _ll_scmple(ll a, ll b)       { return scmp(a, b) <= 0; }

LL _ll_from_l(long a)            { return mk((unsigned)a, a < 0 ? ~0u : 0); }
LL _ll_from_u(unsigned long a)   { return mk(a, 0); }
long _ll_to_l(ll a)              { return (long)a.lo; }
LL _ll_sfrom_d(double d)         { return sfromd(d); }
LL _ll_sfrom_f(float f)          { return sfromd(f); }
LL _ll_ufrom_d(double d)         { return ufromd(d); }
LL _ll_ufrom_f(float f)          { return ufromd(f); }
LL _ll_sto_d(ll a)               { return dresult(stod(a)); }
LL _ll_uto_d(ll a)               { return dresult(utod(a)); }
unsigned _ll_sto_f(ll a)         { return fresult((float)stod(a)); }
unsigned _ll_uto_f(ll a)         { return fresult((float)utod(a)); }

/* end of runtime/i386/llong.c */
