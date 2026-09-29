/* Copyright 2025 Piers Wombwell
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// ieeeflt.c — tiny IEEE helpers used by CSE for constant folding
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

#include "host.h"
#include "ieeeflt.h"
#include "int64.h"

static double dblebin_get_double(const DbleBin *db)
{
    DblePun u;
    u.b.lsd = db->lsd;
    u.b.msd = db->msd;
    return u.d;
}

static void dblebin_set_double(DbleBin *db, double x)
{
    DblePun u;
    u.d = x;
    db->lsd = u.b.lsd;
    db->msd = u.b.msd;
}

int my_isnan(const DbleBin *d) {
    /* exp all 1s and mantissa nonzero => NaN */
    unsigned int exp = (d->msd >> 20) & 0x7FFu;
    unsigned int manthi = d->msd & 0xFFFFFu;
    return (exp == 0x7FFu) && (manthi | d->lsd);
}

int my_isfinite(const DbleBin *d) {
    unsigned int exp = (d->msd >> 20) & 0x7FFu;
    return exp != 0x7FFu;
}

static inline int status(DbleBin *d) {
    if (my_isnan(d)) return flt_invalidop;
    if (!my_isfinite(d)) return flt_very_big;
    return flt_ok;
}

// float (single-precision)
int my_fisnan(const FloatBin *f) {
    unsigned int v = f->val;
    unsigned int exp = (v >> 23) & 0xFFu;
    unsigned int mant = v & 0x7FFFFFu;
    return (exp == 0xFFu) && (mant != 0);
}

int my_fisfinite(const FloatBin *f) {
    unsigned int v = f->val;
    unsigned int exp = (v >> 23) & 0xFFu;
    return exp != 0xFFu;
}

// core arithmetic
int flt_add(DbleBin *res, const DbleBin *a1, const DbleBin *a2)
{
    double x = dblebin_get_double(a1);
    double y = dblebin_get_double(a2);

    dblebin_set_double(res, x + y);
    return status(res);
}

int flt_subtract(DbleBin *res, const DbleBin *a1, const DbleBin *a2)
{
    double x = dblebin_get_double(a1);
    double y = dblebin_get_double(a2);

    dblebin_set_double(res, x - y);
    return status(res);
}

int flt_multiply(DbleBin *res, const DbleBin *a1, const DbleBin *a2)
{
    double x = dblebin_get_double(a1);
    double y = dblebin_get_double(a2);

    dblebin_set_double(res, x * y);
    return status(res);
}

int flt_divide(DbleBin *res, const DbleBin *a1, const DbleBin *a2)
{
    double x = dblebin_get_double(a1);
    double y = dblebin_get_double(a2);

    /* division by zero gives an infinity or a NaN, as IEEE says */
    dblebin_set_double(res, x / y);
    return status(res);
}

int flt_fmod(DbleBin *res, const DbleBin *a1, const DbleBin *a2)
{
    double x = dblebin_get_double(a1);
    double y = dblebin_get_double(a2);

    dblebin_set_double(res, fmod(x, y));
    return status(res);
}

int flt_invert(DbleBin *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    dblebin_set_double(res, 1.0 / x);
    return status(res);
}

int flt_negate(DbleBin *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    dblebin_set_double(res, -x);
    return flt_ok;
}

int flt_abs(DbleBin *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    dblebin_set_double(res, fabs(x));
    return flt_ok;
}

int flt_floor(DbleBin *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    dblebin_set_double(res, floor(x));
    return status(res);
}

int flt_ceil(DbleBin *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    dblebin_set_double(res, ceil(x));
    return status(res);
}

// comparisons
int flt_compare(const DbleBin *a1, const DbleBin *a2)
{
    if (my_isnan(a1) || my_isnan(a2))
        return 42;

    {
        double x = dblebin_get_double(a1);
        double y = dblebin_get_double(a2);

        if (x < y) return -1;
        if (x > y) return +1;
        return 0;
    }
}

// conversions
/* Conversions to integers truncate towards zero.  C leaves a value that
 * does not fit undefined; here it saturates and a NaN gives zero, which
 * is what the MEOW runtime does, and the status says so. */
int flt_dtoi(int32_t *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    if (my_isnan(a1)) { *res = 0; return flt_invalidop; }
    if (x >= 2147483648.0) { *res = INT32_MAX; return flt_inexact; }
    if (x < -2147483648.0) { *res = INT32_MIN; return flt_inexact; }
    *res = (int32_t)x;
    return flt_ok;
}

int flt_dtou(uint32_t *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);

    if (my_isnan(a1)) { *res = 0; return flt_invalidop; }
    if (x >= 4294967296.0) { *res = UINT32_MAX; return flt_inexact; }
    if (x <= -1.0) { *res = 0; return flt_inexact; }
    *res = (uint32_t)x;
    return flt_ok;
}

int flt_dtoll(int64 *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);
    long long v;
    int status = flt_ok;

    if (my_isnan(a1)) { v = 0; status = flt_invalidop; }
    else if (x >= 9223372036854775808.0) { v = LLONG_MAX; status = flt_inexact; }
    else if (x < -9223372036854775808.0) { v = LLONG_MIN; status = flt_inexact; }
    else v = (long long)x;
    res->lo = (uint32)(unsigned long long)v;
    res->hi = (int32)((unsigned long long)v >> 32);
    return status;
}

int flt_dtoull(uint64 *res, const DbleBin *a1)
{
    double x = dblebin_get_double(a1);
    unsigned long long v;
    int status = flt_ok;

    if (my_isnan(a1)) { v = 0; status = flt_invalidop; }
    else if (x >= 18446744073709551616.0) { v = ULLONG_MAX; status = flt_inexact; }
    else if (x <= -1.0) { v = 0; status = flt_inexact; }
    else v = (unsigned long long)x;
    res->lo = (uint32)v;
    res->hi = (uint32)(v >> 32);
    return status;
}

int flt_itod(DbleBin *res, int32_t n)
{
    dblebin_set_double(res, (double)n);
    return flt_ok;
}

int flt_utod(DbleBin *res, uint32_t n)
{
    dblebin_set_double(res, (double)n);
    return flt_ok;
}

int flt_lltod(DbleBin *res, const int64 *n)
{
    long long v = (long long)(((unsigned long long)(uint32)n->hi << 32) | n->lo);
    dblebin_set_double(res, (double)v);
    return flt_ok;
}

int flt_ulltod(DbleBin *res, const uint64 *n)
{
    unsigned long long v = ((unsigned long long)n->hi << 32) | n->lo;
    dblebin_set_double(res, (double)v);
    return flt_ok;
}

int flt_move(DbleBin *res, const DbleBin *a1) {
    *res = *a1;
    return flt_ok;
}

void fltrep_widen(const FloatBin *a, DbleBin *res)
{
    dblebin_set_double(res, (double)a->f);
}

/* Narrowing rounds to nearest; the status only remarks on a finite value
 * that became an infinity or a nonzero value that became zero.  A
 * denormal result is a value like any other. */
int fltrep_narrow(const DbleBin *a, FloatBin *res)
{
    double x = dblebin_get_double(a);
    res->f = (float)x;

    if (!my_isfinite(a))
        return flt_ok;
    if (!my_fisfinite(res))
        return flt_big_single;
    if (x != 0.0 && res->f == 0.0f)
        return flt_small_single;
    return flt_ok;
}

int fltrep_narrow_round(const DbleBin *a, FloatBin *res) {
    // FIXME: Not sure what the difference is.
    return fltrep_narrow(a, res);
}

int fltrep_stod(const char *s, DbleBin *out, void *ignored)
{
    char *endp;
    double x;

    IGNORE(ignored);
    if (!s || !out) return flt_bad;

    errno = 0;
    endp = NULL;
    x = strtod(s, &endp);

    if (endp == s) {
        dblebin_set_double(out, 0.0);
        return flt_bad;
    }
    /* the value is kept whatever happened: an infinity for overflow, a
     * denormal or zero for underflow */
    dblebin_set_double(out, x);
    if (x == HUGE_VAL || x == -HUGE_VAL)
        return flt_very_big;
    if (errno == ERANGE && x == 0.0)
        return flt_very_small;
    return flt_ok;
}

void fltrep_sprintf(char *res, const char *fmt, const DbleBin *a)
{
    // Usage in tree: fltrep_sprintf(buf, "%g", &db), so this isn't a vararg.

    // We don't know the length, so let's potentially blat over something!
    double x = dblebin_get_double(a);
    (void)sprintf(res, fmt, x);
}
