// SSE2 floating point, conversions, and float results.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run
// KNOWN-FAIL: ordered compares are not IEEE-correct for NaN, as mip negates conditions

int printf(const char *, ...);
double dadd(double a, double b) { return a + b; }
float fmul(float a, float b) { return a * b; }
double poly(double x) { return ((2.5 * x - 1.25) * x + 0.5) / (x - 3.0); }
int lt(double a, double b) { return a < b; }
int le(double a, double b) { return a <= b; }
int eq(double a, double b) { return a == b; }
int ne(double a, double b) { return a != b; }
int ge(double a, double b) { return a >= b; }
int gt(double a, double b) { return a > b; }
double neg(double a) { return -a; }
float fneg(float a) { return -a; }
int toint(double d) { return (int)d; }
double fromint(int i) { return i; }
float narrow(double d) { return (float)d; }
double widen(float f) { return f; }
double sub(double a, double b) { return a - b; }
double rsub(double a, double b) { return b - a; }
double sum(double *p, int n) { double s = 0; while (n--) s += *p++; return s; }
int main(void)
{   double z = 0.0, nan = z / z, arr[4] = { 1.5, 2.25, -3.0, 0.125 };
    double v[] = { -1.0, 0.0, 2.0, 3.5 };
    int i, j;
    printf("%g %g %g\n", dadd(1.5, 2.25), (double)fmul(1.5f, 4.0f), poly(1.0));
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
        printf("%d%d%d%d%d%d ", lt(v[i], v[j]), le(v[i], v[j]), eq(v[i], v[j]),
               ne(v[i], v[j]), ge(v[i], v[j]), gt(v[i], v[j]));
    printf("\nnan: %d%d%d%d%d%d\n", lt(nan, 1), le(nan, 1), eq(nan, nan), ne(nan, nan), ge(nan, 1), gt(nan, 1));
    printf("%g %g %d %g %g %g\n", neg(2.5), (double)fneg(-1.5f), toint(-7.9), fromint(-3), (double)narrow(1.0/3.0), widen(0.1f));
    printf("%g %g %g\n", sub(10, 4), rsub(10, 4), sum(arr, 4));
    if (!(nan < 1.0)) printf("not less ok\n");
    if (nan != nan) printf("ne ok\n");
    return 0;
}

// CHECK: 3.75 6 -0.875
// CHECK: 011010 110100 110100 110100 000111 011010 110100 110100 000111 000111 011010 110100 000111 000111 000111 011010 
// CHECK: nan: 000100
// CHECK: -2.5 1.5 -7 -3 0.333333 0.1
// CHECK: 6 -6 0.875
// CHECK: not less ok
// CHECK: ne ok
