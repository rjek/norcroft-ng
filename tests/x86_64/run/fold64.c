// Constant folding of 32- and 64-bit integer expressions (generated):
// each is compared with the same expression evaluated at run time.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
static int bad;
static void check(int n, unsigned long folded, unsigned long run)
{   if (folded != run) printf("%d: %lx != %lx\n", n, folded, run), bad++;
}
static void checkd(int n, double folded, double run)
{   if (folded != run) printf("%d: %.17g != %.17g\n", n, folded, run), bad++;
}
int main(void)
{
    { volatile int a = (int)(0xfffffffffffffff0); volatile unsigned long b = (unsigned long)(1);
      check(0, (unsigned long)((int)(0xfffffffffffffff0) * (unsigned long)(1)), (unsigned long)(a * b));
      check(0, (unsigned long)~(int)(0xfffffffffffffff0), (unsigned long)~a);
      check(0, (unsigned long)-(int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(0, (double)(int)(0xfffffffffffffff0), (double)a); }
    { volatile long a = (long)(1); volatile int b = (int)(63);
      check(1, (unsigned long)((long)(1) | (int)(63)), (unsigned long)(a | b));
      check(1, (unsigned long)~(long)(1), (unsigned long)~a);
      check(1, (unsigned long)-(long)(1), (unsigned long)-a);
      checkd(1, (double)(long)(1), (double)a); }
    { volatile long a = (long)(12345678901); volatile long b = (long)(12345678901);
      check(2, (unsigned long)((long)(12345678901) * (long)(12345678901)), (unsigned long)(a * b));
      check(2, (unsigned long)~(long)(12345678901), (unsigned long)~a);
      check(2, (unsigned long)-(long)(12345678901), (unsigned long)-a);
      checkd(2, (double)(long)(12345678901), (double)a); }
    { volatile unsigned long a = (unsigned long)(12345678901); volatile long b = (long)(40);
      check(3, (unsigned long)((unsigned long)(12345678901) / (long)(40)), (unsigned long)(a / b));
      check(3, (unsigned long)~(unsigned long)(12345678901), (unsigned long)~a);
      check(3, (unsigned long)-(unsigned long)(12345678901), (unsigned long)-a);
      checkd(3, (double)(unsigned long)(12345678901), (double)a); }
    { volatile long a = (long)(1); volatile unsigned int b = (unsigned int)(0xffffffff);
      check(4, (unsigned long)((long)(1) - (unsigned int)(0xffffffff)), (unsigned long)(a - b));
      check(4, (unsigned long)~(long)(1), (unsigned long)~a);
      check(4, (unsigned long)-(long)(1), (unsigned long)-a);
      checkd(4, (double)(long)(1), (double)a); }
    { volatile unsigned long a = (unsigned long)(12345678901); volatile int b = (int)(63);
      check(5, (unsigned long)((unsigned long)(12345678901) / (int)(63)), (unsigned long)(a / b));
      check(5, (unsigned long)~(unsigned long)(12345678901), (unsigned long)~a);
      check(5, (unsigned long)-(unsigned long)(12345678901), (unsigned long)-a);
      checkd(5, (double)(unsigned long)(12345678901), (double)a); }
    { volatile long a = (long)(40); volatile unsigned long b = (unsigned long)(12345678901);
      check(6, (unsigned long)((long)(40) < (unsigned long)(12345678901)), (unsigned long)(a < b));
      check(6, (unsigned long)~(long)(40), (unsigned long)~a);
      check(6, (unsigned long)-(long)(40), (unsigned long)-a);
      checkd(6, (double)(long)(40), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x8000000000000000); volatile unsigned int b = (unsigned int)(-0x80000000);
      check(7, (unsigned long)((unsigned int)(0x8000000000000000) ^ (unsigned int)(-0x80000000)), (unsigned long)(a ^ b));
      check(7, (unsigned long)~(unsigned int)(0x8000000000000000), (unsigned long)~a);
      check(7, (unsigned long)-(unsigned int)(0x8000000000000000), (unsigned long)-a);
      checkd(7, (double)(unsigned int)(0x8000000000000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(-1); volatile unsigned long b = (unsigned long)(-0x80000000);
      check(8, (unsigned long)((unsigned long)(-1) >= (unsigned long)(-0x80000000)), (unsigned long)(a >= b));
      check(8, (unsigned long)~(unsigned long)(-1), (unsigned long)~a);
      check(8, (unsigned long)-(unsigned long)(-1), (unsigned long)-a);
      checkd(8, (double)(unsigned long)(-1), (double)a); }
    { volatile unsigned long a = (unsigned long)(31); volatile int b = (int)(40);
      check(9, (unsigned long)((unsigned long)(31) != (int)(40)), (unsigned long)(a != b));
      check(9, (unsigned long)~(unsigned long)(31), (unsigned long)~a);
      check(9, (unsigned long)-(unsigned long)(31), (unsigned long)-a);
      checkd(9, (double)(unsigned long)(31), (double)a); }
    { volatile long a = (long)(0x7fffffffffffffff); volatile long b = (long)(0x7fffffffffffffff);
      check(10, (unsigned long)((long)(0x7fffffffffffffff) <= (long)(0x7fffffffffffffff)), (unsigned long)(a <= b));
      check(10, (unsigned long)~(long)(0x7fffffffffffffff), (unsigned long)~a);
      check(10, (unsigned long)-(long)(0x7fffffffffffffff), (unsigned long)-a);
      checkd(10, (double)(long)(0x7fffffffffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(-1); volatile int b = (int)(31);
      check(11, (unsigned long)((unsigned int)(-1) << (int)(31)), (unsigned long)(a << b));
      check(11, (unsigned long)~(unsigned int)(-1), (unsigned long)~a);
      check(11, (unsigned long)-(unsigned int)(-1), (unsigned long)-a);
      checkd(11, (double)(unsigned int)(-1), (double)a); }
    { volatile long a = (long)(-0x80000000); volatile int b = (int)(40);
      check(12, (unsigned long)((long)(-0x80000000) >> (int)(40)), (unsigned long)(a >> b));
      check(12, (unsigned long)~(long)(-0x80000000), (unsigned long)~a);
      check(12, (unsigned long)-(long)(-0x80000000), (unsigned long)-a);
      checkd(12, (double)(long)(-0x80000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(0); volatile int b = (int)(-12345678901);
      check(13, (unsigned long)((unsigned int)(0) <= (int)(-12345678901)), (unsigned long)(a <= b));
      check(13, (unsigned long)~(unsigned int)(0), (unsigned long)~a);
      check(13, (unsigned long)-(unsigned int)(0), (unsigned long)-a);
      checkd(13, (double)(unsigned int)(0), (double)a); }
    { volatile unsigned long a = (unsigned long)(40); volatile long b = (long)(1);
      check(14, (unsigned long)((unsigned long)(40) | (long)(1)), (unsigned long)(a | b));
      check(14, (unsigned long)~(unsigned long)(40), (unsigned long)~a);
      check(14, (unsigned long)-(unsigned long)(40), (unsigned long)-a);
      checkd(14, (double)(unsigned long)(40), (double)a); }
    { volatile int a = (int)(0xffffffff); volatile unsigned long b = (unsigned long)(0xfffffffffffffff0);
      check(15, (unsigned long)((int)(0xffffffff) == (unsigned long)(0xfffffffffffffff0)), (unsigned long)(a == b));
      check(15, (unsigned long)~(int)(0xffffffff), (unsigned long)~a);
      check(15, (unsigned long)-(int)(0xffffffff), (unsigned long)-a);
      checkd(15, (double)(int)(0xffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x7fffffff); volatile long b = (long)(-12345678901);
      check(16, (unsigned long)((unsigned int)(0x7fffffff) == (long)(-12345678901)), (unsigned long)(a == b));
      check(16, (unsigned long)~(unsigned int)(0x7fffffff), (unsigned long)~a);
      check(16, (unsigned long)-(unsigned int)(0x7fffffff), (unsigned long)-a);
      checkd(16, (double)(unsigned int)(0x7fffffff), (double)a); }
    { volatile int a = (int)(12345678901); volatile unsigned long b = (unsigned long)(0x100000000);
      check(17, (unsigned long)((int)(12345678901) != (unsigned long)(0x100000000)), (unsigned long)(a != b));
      check(17, (unsigned long)~(int)(12345678901), (unsigned long)~a);
      check(17, (unsigned long)-(int)(12345678901), (unsigned long)-a);
      checkd(17, (double)(int)(12345678901), (double)a); }
    { volatile int a = (int)(0xffffffff); volatile unsigned int b = (unsigned int)(31);
      check(18, (unsigned long)((int)(0xffffffff) * (unsigned int)(31)), (unsigned long)(a * b));
      check(18, (unsigned long)~(int)(0xffffffff), (unsigned long)~a);
      check(18, (unsigned long)-(int)(0xffffffff), (unsigned long)-a);
      checkd(18, (double)(int)(0xffffffff), (double)a); }
    { volatile unsigned long a = (unsigned long)(0xffffffff); volatile unsigned long b = (unsigned long)(0xffffffff);
      check(19, (unsigned long)((unsigned long)(0xffffffff) + (unsigned long)(0xffffffff)), (unsigned long)(a + b));
      check(19, (unsigned long)~(unsigned long)(0xffffffff), (unsigned long)~a);
      check(19, (unsigned long)-(unsigned long)(0xffffffff), (unsigned long)-a);
      checkd(19, (double)(unsigned long)(0xffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x100000000); volatile unsigned long b = (unsigned long)(-0x80000000);
      check(20, (unsigned long)((unsigned int)(0x100000000) + (unsigned long)(-0x80000000)), (unsigned long)(a + b));
      check(20, (unsigned long)~(unsigned int)(0x100000000), (unsigned long)~a);
      check(20, (unsigned long)-(unsigned int)(0x100000000), (unsigned long)-a);
      checkd(20, (double)(unsigned int)(0x100000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xfffffffffffffff0); volatile unsigned int b = (unsigned int)(0xfffffffffffffff0);
      check(21, (unsigned long)((unsigned int)(0xfffffffffffffff0) == (unsigned int)(0xfffffffffffffff0)), (unsigned long)(a == b));
      check(21, (unsigned long)~(unsigned int)(0xfffffffffffffff0), (unsigned long)~a);
      check(21, (unsigned long)-(unsigned int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(21, (double)(unsigned int)(0xfffffffffffffff0), (double)a); }
    { volatile long a = (long)(0xfffffffffffffff0); volatile unsigned int b = (unsigned int)(1);
      check(22, (unsigned long)((long)(0xfffffffffffffff0) | (unsigned int)(1)), (unsigned long)(a | b));
      check(22, (unsigned long)~(long)(0xfffffffffffffff0), (unsigned long)~a);
      check(22, (unsigned long)-(long)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(22, (double)(long)(0xfffffffffffffff0), (double)a); }
    { volatile long a = (long)(-12345678901); volatile unsigned long b = (unsigned long)(12345678901);
      check(23, (unsigned long)((long)(-12345678901) / (unsigned long)(12345678901)), (unsigned long)(a / b));
      check(23, (unsigned long)~(long)(-12345678901), (unsigned long)~a);
      check(23, (unsigned long)-(long)(-12345678901), (unsigned long)-a);
      checkd(23, (double)(long)(-12345678901), (double)a); }
    { volatile long a = (long)(0); volatile long b = (long)(12345678901);
      check(24, (unsigned long)((long)(0) / (long)(12345678901)), (unsigned long)(a / b));
      check(24, (unsigned long)~(long)(0), (unsigned long)~a);
      check(24, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(24, (double)(long)(0), (double)a); }
    { volatile int a = (int)(40); volatile int b = (int)(-12345678901);
      check(25, (unsigned long)((int)(40) / (int)(-12345678901)), (unsigned long)(a / b));
      check(25, (unsigned long)~(int)(40), (unsigned long)~a);
      check(25, (unsigned long)-(int)(40), (unsigned long)-a);
      checkd(25, (double)(int)(40), (double)a); }
    { volatile unsigned int a = (unsigned int)(40); volatile unsigned int b = (unsigned int)(-0x80000000);
      check(26, (unsigned long)((unsigned int)(40) * (unsigned int)(-0x80000000)), (unsigned long)(a * b));
      check(26, (unsigned long)~(unsigned int)(40), (unsigned long)~a);
      check(26, (unsigned long)-(unsigned int)(40), (unsigned long)-a);
      checkd(26, (double)(unsigned int)(40), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x7fffffffffffffff); volatile long b = (long)(0x100000000);
      check(27, (unsigned long)((unsigned long)(0x7fffffffffffffff) >= (long)(0x100000000)), (unsigned long)(a >= b));
      check(27, (unsigned long)~(unsigned long)(0x7fffffffffffffff), (unsigned long)~a);
      check(27, (unsigned long)-(unsigned long)(0x7fffffffffffffff), (unsigned long)-a);
      checkd(27, (double)(unsigned long)(0x7fffffffffffffff), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x80000000); volatile long b = (long)(63);
      check(28, (unsigned long)((unsigned long)(0x80000000) <= (long)(63)), (unsigned long)(a <= b));
      check(28, (unsigned long)~(unsigned long)(0x80000000), (unsigned long)~a);
      check(28, (unsigned long)-(unsigned long)(0x80000000), (unsigned long)-a);
      checkd(28, (double)(unsigned long)(0x80000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(63); volatile long b = (long)(-0x80000000);
      check(29, (unsigned long)((unsigned long)(63) * (long)(-0x80000000)), (unsigned long)(a * b));
      check(29, (unsigned long)~(unsigned long)(63), (unsigned long)~a);
      check(29, (unsigned long)-(unsigned long)(63), (unsigned long)-a);
      checkd(29, (double)(unsigned long)(63), (double)a); }
    { volatile int a = (int)(0x7fffffff); volatile int b = (int)(0x8000000000000000);
      check(30, (unsigned long)((int)(0x7fffffff) ^ (int)(0x8000000000000000)), (unsigned long)(a ^ b));
      check(30, (unsigned long)~(int)(0x7fffffff), (unsigned long)~a);
      check(30, (unsigned long)-(int)(0x7fffffff), (unsigned long)-a);
      checkd(30, (double)(int)(0x7fffffff), (double)a); }
    { volatile int a = (int)(0x80000000); volatile unsigned long b = (unsigned long)(0xffffffff);
      check(31, (unsigned long)((int)(0x80000000) == (unsigned long)(0xffffffff)), (unsigned long)(a == b));
      check(31, (unsigned long)~(int)(0x80000000), (unsigned long)~a);
      check(31, (unsigned long)-(int)(0x80000000), (unsigned long)-a);
      checkd(31, (double)(int)(0x80000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(63); volatile unsigned long b = (unsigned long)(40);
      check(32, (unsigned long)((unsigned long)(63) <= (unsigned long)(40)), (unsigned long)(a <= b));
      check(32, (unsigned long)~(unsigned long)(63), (unsigned long)~a);
      check(32, (unsigned long)-(unsigned long)(63), (unsigned long)-a);
      checkd(32, (double)(unsigned long)(63), (double)a); }
    { volatile long a = (long)(0x100000000); volatile int b = (int)(1);
      check(33, (unsigned long)((long)(0x100000000) << (int)(1)), (unsigned long)(a << b));
      check(33, (unsigned long)~(long)(0x100000000), (unsigned long)~a);
      check(33, (unsigned long)-(long)(0x100000000), (unsigned long)-a);
      checkd(33, (double)(long)(0x100000000), (double)a); }
    { volatile int a = (int)(0x8000000000000000); volatile unsigned int b = (unsigned int)(0x8000000000000000);
      check(34, (unsigned long)((int)(0x8000000000000000) * (unsigned int)(0x8000000000000000)), (unsigned long)(a * b));
      check(34, (unsigned long)~(int)(0x8000000000000000), (unsigned long)~a);
      check(34, (unsigned long)-(int)(0x8000000000000000), (unsigned long)-a);
      checkd(34, (double)(int)(0x8000000000000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(0xffffffff); volatile long b = (long)(40);
      check(35, (unsigned long)((unsigned long)(0xffffffff) | (long)(40)), (unsigned long)(a | b));
      check(35, (unsigned long)~(unsigned long)(0xffffffff), (unsigned long)~a);
      check(35, (unsigned long)-(unsigned long)(0xffffffff), (unsigned long)-a);
      checkd(35, (double)(unsigned long)(0xffffffff), (double)a); }
    { volatile int a = (int)(40); volatile unsigned long b = (unsigned long)(0);
      check(36, (unsigned long)((int)(40) >= (unsigned long)(0)), (unsigned long)(a >= b));
      check(36, (unsigned long)~(int)(40), (unsigned long)~a);
      check(36, (unsigned long)-(int)(40), (unsigned long)-a);
      checkd(36, (double)(int)(40), (double)a); }
    { volatile int a = (int)(2); volatile long b = (long)(0xfffffffffffffff0);
      check(37, (unsigned long)((int)(2) | (long)(0xfffffffffffffff0)), (unsigned long)(a | b));
      check(37, (unsigned long)~(int)(2), (unsigned long)~a);
      check(37, (unsigned long)-(int)(2), (unsigned long)-a);
      checkd(37, (double)(int)(2), (double)a); }
    { volatile unsigned int a = (unsigned int)(12345678901); volatile unsigned long b = (unsigned long)(0x7fffffffffffffff);
      check(38, (unsigned long)((unsigned int)(12345678901) * (unsigned long)(0x7fffffffffffffff)), (unsigned long)(a * b));
      check(38, (unsigned long)~(unsigned int)(12345678901), (unsigned long)~a);
      check(38, (unsigned long)-(unsigned int)(12345678901), (unsigned long)-a);
      checkd(38, (double)(unsigned int)(12345678901), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xfffffffffffffff0); volatile unsigned int b = (unsigned int)(-1);
      check(39, (unsigned long)((unsigned int)(0xfffffffffffffff0) & (unsigned int)(-1)), (unsigned long)(a & b));
      check(39, (unsigned long)~(unsigned int)(0xfffffffffffffff0), (unsigned long)~a);
      check(39, (unsigned long)-(unsigned int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(39, (double)(unsigned int)(0xfffffffffffffff0), (double)a); }
    { volatile unsigned long a = (unsigned long)(0); volatile unsigned long b = (unsigned long)(31);
      check(40, (unsigned long)((unsigned long)(0) > (unsigned long)(31)), (unsigned long)(a > b));
      check(40, (unsigned long)~(unsigned long)(0), (unsigned long)~a);
      check(40, (unsigned long)-(unsigned long)(0), (unsigned long)-a);
      checkd(40, (double)(unsigned long)(0), (double)a); }
    { volatile long a = (long)(63); volatile long b = (long)(31);
      check(41, (unsigned long)((long)(63) != (long)(31)), (unsigned long)(a != b));
      check(41, (unsigned long)~(long)(63), (unsigned long)~a);
      check(41, (unsigned long)-(long)(63), (unsigned long)-a);
      checkd(41, (double)(long)(63), (double)a); }
    { volatile unsigned long a = (unsigned long)(0); volatile unsigned long b = (unsigned long)(0x100000000);
      check(42, (unsigned long)((unsigned long)(0) | (unsigned long)(0x100000000)), (unsigned long)(a | b));
      check(42, (unsigned long)~(unsigned long)(0), (unsigned long)~a);
      check(42, (unsigned long)-(unsigned long)(0), (unsigned long)-a);
      checkd(42, (double)(unsigned long)(0), (double)a); }
    { volatile int a = (int)(0x7fffffffffffffff); volatile unsigned long b = (unsigned long)(0x100000000);
      check(43, (unsigned long)((int)(0x7fffffffffffffff) != (unsigned long)(0x100000000)), (unsigned long)(a != b));
      check(43, (unsigned long)~(int)(0x7fffffffffffffff), (unsigned long)~a);
      check(43, (unsigned long)-(int)(0x7fffffffffffffff), (unsigned long)-a);
      checkd(43, (double)(int)(0x7fffffffffffffff), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x8000000000000000); volatile long b = (long)(-12345678901);
      check(44, (unsigned long)((unsigned long)(0x8000000000000000) != (long)(-12345678901)), (unsigned long)(a != b));
      check(44, (unsigned long)~(unsigned long)(0x8000000000000000), (unsigned long)~a);
      check(44, (unsigned long)-(unsigned long)(0x8000000000000000), (unsigned long)-a);
      checkd(44, (double)(unsigned long)(0x8000000000000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(63); volatile unsigned long b = (unsigned long)(63);
      check(45, (unsigned long)((unsigned long)(63) + (unsigned long)(63)), (unsigned long)(a + b));
      check(45, (unsigned long)~(unsigned long)(63), (unsigned long)~a);
      check(45, (unsigned long)-(unsigned long)(63), (unsigned long)-a);
      checkd(45, (double)(unsigned long)(63), (double)a); }
    { volatile unsigned int a = (unsigned int)(0); volatile unsigned long b = (unsigned long)(31);
      check(46, (unsigned long)((unsigned int)(0) & (unsigned long)(31)), (unsigned long)(a & b));
      check(46, (unsigned long)~(unsigned int)(0), (unsigned long)~a);
      check(46, (unsigned long)-(unsigned int)(0), (unsigned long)-a);
      checkd(46, (double)(unsigned int)(0), (double)a); }
    { volatile unsigned long a = (unsigned long)(2); volatile unsigned int b = (unsigned int)(1);
      check(47, (unsigned long)((unsigned long)(2) < (unsigned int)(1)), (unsigned long)(a < b));
      check(47, (unsigned long)~(unsigned long)(2), (unsigned long)~a);
      check(47, (unsigned long)-(unsigned long)(2), (unsigned long)-a);
      checkd(47, (double)(unsigned long)(2), (double)a); }
    { volatile unsigned int a = (unsigned int)(1); volatile long b = (long)(0xffffffff);
      check(48, (unsigned long)((unsigned int)(1) | (long)(0xffffffff)), (unsigned long)(a | b));
      check(48, (unsigned long)~(unsigned int)(1), (unsigned long)~a);
      check(48, (unsigned long)-(unsigned int)(1), (unsigned long)-a);
      checkd(48, (double)(unsigned int)(1), (double)a); }
    { volatile int a = (int)(2); volatile long b = (long)(63);
      check(49, (unsigned long)((int)(2) > (long)(63)), (unsigned long)(a > b));
      check(49, (unsigned long)~(int)(2), (unsigned long)~a);
      check(49, (unsigned long)-(int)(2), (unsigned long)-a);
      checkd(49, (double)(int)(2), (double)a); }
    { volatile long a = (long)(-12345678901); volatile long b = (long)(0x7fffffffffffffff);
      check(50, (unsigned long)((long)(-12345678901) | (long)(0x7fffffffffffffff)), (unsigned long)(a | b));
      check(50, (unsigned long)~(long)(-12345678901), (unsigned long)~a);
      check(50, (unsigned long)-(long)(-12345678901), (unsigned long)-a);
      checkd(50, (double)(long)(-12345678901), (double)a); }
    { volatile int a = (int)(63); volatile unsigned int b = (unsigned int)(40);
      check(51, (unsigned long)((int)(63) ^ (unsigned int)(40)), (unsigned long)(a ^ b));
      check(51, (unsigned long)~(int)(63), (unsigned long)~a);
      check(51, (unsigned long)-(int)(63), (unsigned long)-a);
      checkd(51, (double)(int)(63), (double)a); }
    { volatile int a = (int)(-12345678901); volatile unsigned long b = (unsigned long)(31);
      check(52, (unsigned long)((int)(-12345678901) != (unsigned long)(31)), (unsigned long)(a != b));
      check(52, (unsigned long)~(int)(-12345678901), (unsigned long)~a);
      check(52, (unsigned long)-(int)(-12345678901), (unsigned long)-a);
      checkd(52, (double)(int)(-12345678901), (double)a); }
    { volatile long a = (long)(-12345678901); volatile unsigned int b = (unsigned int)(0x7fffffffffffffff);
      check(53, (unsigned long)((long)(-12345678901) * (unsigned int)(0x7fffffffffffffff)), (unsigned long)(a * b));
      check(53, (unsigned long)~(long)(-12345678901), (unsigned long)~a);
      check(53, (unsigned long)-(long)(-12345678901), (unsigned long)-a);
      checkd(53, (double)(long)(-12345678901), (double)a); }
    { volatile unsigned long a = (unsigned long)(-1); volatile int b = (int)(63);
      check(54, (unsigned long)((unsigned long)(-1) >> (int)(63)), (unsigned long)(a >> b));
      check(54, (unsigned long)~(unsigned long)(-1), (unsigned long)~a);
      check(54, (unsigned long)-(unsigned long)(-1), (unsigned long)-a);
      checkd(54, (double)(unsigned long)(-1), (double)a); }
    { volatile long a = (long)(0x8000000000000000); volatile int b = (int)(1);
      check(55, (unsigned long)((long)(0x8000000000000000) << (int)(1)), (unsigned long)(a << b));
      check(55, (unsigned long)~(long)(0x8000000000000000), (unsigned long)~a);
      check(55, (unsigned long)-(long)(0x8000000000000000), (unsigned long)-a);
      checkd(55, (double)(long)(0x8000000000000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(2); volatile unsigned long b = (unsigned long)(0xfffffffffffffff0);
      check(56, (unsigned long)((unsigned int)(2) >= (unsigned long)(0xfffffffffffffff0)), (unsigned long)(a >= b));
      check(56, (unsigned long)~(unsigned int)(2), (unsigned long)~a);
      check(56, (unsigned long)-(unsigned int)(2), (unsigned long)-a);
      checkd(56, (double)(unsigned int)(2), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x7fffffff); volatile unsigned long b = (unsigned long)(12345678901);
      check(57, (unsigned long)((unsigned long)(0x7fffffff) == (unsigned long)(12345678901)), (unsigned long)(a == b));
      check(57, (unsigned long)~(unsigned long)(0x7fffffff), (unsigned long)~a);
      check(57, (unsigned long)-(unsigned long)(0x7fffffff), (unsigned long)-a);
      checkd(57, (double)(unsigned long)(0x7fffffff), (double)a); }
    { volatile int a = (int)(0x80000000); volatile unsigned int b = (unsigned int)(0x8000000000000000);
      check(58, (unsigned long)((int)(0x80000000) < (unsigned int)(0x8000000000000000)), (unsigned long)(a < b));
      check(58, (unsigned long)~(int)(0x80000000), (unsigned long)~a);
      check(58, (unsigned long)-(int)(0x80000000), (unsigned long)-a);
      checkd(58, (double)(int)(0x80000000), (double)a); }
    { volatile long a = (long)(0); volatile int b = (int)(0x7fffffffffffffff);
      check(59, (unsigned long)((long)(0) > (int)(0x7fffffffffffffff)), (unsigned long)(a > b));
      check(59, (unsigned long)~(long)(0), (unsigned long)~a);
      check(59, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(59, (double)(long)(0), (double)a); }
    { volatile long a = (long)(0xffffffff); volatile long b = (long)(2);
      check(60, (unsigned long)((long)(0xffffffff) * (long)(2)), (unsigned long)(a * b));
      check(60, (unsigned long)~(long)(0xffffffff), (unsigned long)~a);
      check(60, (unsigned long)-(long)(0xffffffff), (unsigned long)-a);
      checkd(60, (double)(long)(0xffffffff), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x100000000); volatile unsigned int b = (unsigned int)(-12345678901);
      check(61, (unsigned long)((unsigned long)(0x100000000) % (unsigned int)(-12345678901)), (unsigned long)(a % b));
      check(61, (unsigned long)~(unsigned long)(0x100000000), (unsigned long)~a);
      check(61, (unsigned long)-(unsigned long)(0x100000000), (unsigned long)-a);
      checkd(61, (double)(unsigned long)(0x100000000), (double)a); }
    { volatile int a = (int)(0x100000000); volatile long b = (long)(1);
      check(62, (unsigned long)((int)(0x100000000) & (long)(1)), (unsigned long)(a & b));
      check(62, (unsigned long)~(int)(0x100000000), (unsigned long)~a);
      check(62, (unsigned long)-(int)(0x100000000), (unsigned long)-a);
      checkd(62, (double)(int)(0x100000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x100000000); volatile long b = (long)(0);
      check(63, (unsigned long)((unsigned int)(0x100000000) * (long)(0)), (unsigned long)(a * b));
      check(63, (unsigned long)~(unsigned int)(0x100000000), (unsigned long)~a);
      check(63, (unsigned long)-(unsigned int)(0x100000000), (unsigned long)-a);
      checkd(63, (double)(unsigned int)(0x100000000), (double)a); }
    { volatile long a = (long)(0); volatile unsigned int b = (unsigned int)(0x7fffffffffffffff);
      check(64, (unsigned long)((long)(0) != (unsigned int)(0x7fffffffffffffff)), (unsigned long)(a != b));
      check(64, (unsigned long)~(long)(0), (unsigned long)~a);
      check(64, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(64, (double)(long)(0), (double)a); }
    { volatile int a = (int)(1); volatile unsigned long b = (unsigned long)(63);
      check(65, (unsigned long)((int)(1) ^ (unsigned long)(63)), (unsigned long)(a ^ b));
      check(65, (unsigned long)~(int)(1), (unsigned long)~a);
      check(65, (unsigned long)-(int)(1), (unsigned long)-a);
      checkd(65, (double)(int)(1), (double)a); }
    { volatile long a = (long)(0x100000000); volatile unsigned long b = (unsigned long)(1);
      check(66, (unsigned long)((long)(0x100000000) & (unsigned long)(1)), (unsigned long)(a & b));
      check(66, (unsigned long)~(long)(0x100000000), (unsigned long)~a);
      check(66, (unsigned long)-(long)(0x100000000), (unsigned long)-a);
      checkd(66, (double)(long)(0x100000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(-0x80000000); volatile int b = (int)(63);
      check(67, (unsigned long)((unsigned long)(-0x80000000) | (int)(63)), (unsigned long)(a | b));
      check(67, (unsigned long)~(unsigned long)(-0x80000000), (unsigned long)~a);
      check(67, (unsigned long)-(unsigned long)(-0x80000000), (unsigned long)-a);
      checkd(67, (double)(unsigned long)(-0x80000000), (double)a); }
    { volatile int a = (int)(63); volatile int b = (int)(3);
      check(68, (unsigned long)((int)(63) << (int)(3)), (unsigned long)(a << b));
      check(68, (unsigned long)~(int)(63), (unsigned long)~a);
      check(68, (unsigned long)-(int)(63), (unsigned long)-a);
      checkd(68, (double)(int)(63), (double)a); }
    { volatile long a = (long)(1); volatile int b = (int)(0);
      check(69, (unsigned long)((long)(1) + (int)(0)), (unsigned long)(a + b));
      check(69, (unsigned long)~(long)(1), (unsigned long)~a);
      check(69, (unsigned long)-(long)(1), (unsigned long)-a);
      checkd(69, (double)(long)(1), (double)a); }
    { volatile unsigned long a = (unsigned long)(0xffffffff); volatile unsigned int b = (unsigned int)(0x7fffffff);
      check(70, (unsigned long)((unsigned long)(0xffffffff) / (unsigned int)(0x7fffffff)), (unsigned long)(a / b));
      check(70, (unsigned long)~(unsigned long)(0xffffffff), (unsigned long)~a);
      check(70, (unsigned long)-(unsigned long)(0xffffffff), (unsigned long)-a);
      checkd(70, (double)(unsigned long)(0xffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(63); volatile unsigned int b = (unsigned int)(-0x80000000);
      check(71, (unsigned long)((unsigned int)(63) | (unsigned int)(-0x80000000)), (unsigned long)(a | b));
      check(71, (unsigned long)~(unsigned int)(63), (unsigned long)~a);
      check(71, (unsigned long)-(unsigned int)(63), (unsigned long)-a);
      checkd(71, (double)(unsigned int)(63), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x80000000); volatile int b = (int)(31);
      check(72, (unsigned long)((unsigned long)(0x80000000) == (int)(31)), (unsigned long)(a == b));
      check(72, (unsigned long)~(unsigned long)(0x80000000), (unsigned long)~a);
      check(72, (unsigned long)-(unsigned long)(0x80000000), (unsigned long)-a);
      checkd(72, (double)(unsigned long)(0x80000000), (double)a); }
    { volatile int a = (int)(31); volatile long b = (long)(0);
      check(73, (unsigned long)((int)(31) * (long)(0)), (unsigned long)(a * b));
      check(73, (unsigned long)~(int)(31), (unsigned long)~a);
      check(73, (unsigned long)-(int)(31), (unsigned long)-a);
      checkd(73, (double)(int)(31), (double)a); }
    { volatile int a = (int)(0x7fffffff); volatile unsigned int b = (unsigned int)(1);
      check(74, (unsigned long)((int)(0x7fffffff) * (unsigned int)(1)), (unsigned long)(a * b));
      check(74, (unsigned long)~(int)(0x7fffffff), (unsigned long)~a);
      check(74, (unsigned long)-(int)(0x7fffffff), (unsigned long)-a);
      checkd(74, (double)(int)(0x7fffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xffffffff); volatile int b = (int)(-0x80000000);
      check(75, (unsigned long)((unsigned int)(0xffffffff) - (int)(-0x80000000)), (unsigned long)(a - b));
      check(75, (unsigned long)~(unsigned int)(0xffffffff), (unsigned long)~a);
      check(75, (unsigned long)-(unsigned int)(0xffffffff), (unsigned long)-a);
      checkd(75, (double)(unsigned int)(0xffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x7fffffff); volatile unsigned long b = (unsigned long)(0x100000000);
      check(76, (unsigned long)((unsigned int)(0x7fffffff) > (unsigned long)(0x100000000)), (unsigned long)(a > b));
      check(76, (unsigned long)~(unsigned int)(0x7fffffff), (unsigned long)~a);
      check(76, (unsigned long)-(unsigned int)(0x7fffffff), (unsigned long)-a);
      checkd(76, (double)(unsigned int)(0x7fffffff), (double)a); }
    { volatile long a = (long)(0x8000000000000000); volatile int b = (int)(0x7fffffffffffffff);
      check(77, (unsigned long)((long)(0x8000000000000000) < (int)(0x7fffffffffffffff)), (unsigned long)(a < b));
      check(77, (unsigned long)~(long)(0x8000000000000000), (unsigned long)~a);
      check(77, (unsigned long)-(long)(0x8000000000000000), (unsigned long)-a);
      checkd(77, (double)(long)(0x8000000000000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(-0x80000000); volatile long b = (long)(0x80000000);
      check(78, (unsigned long)((unsigned long)(-0x80000000) <= (long)(0x80000000)), (unsigned long)(a <= b));
      check(78, (unsigned long)~(unsigned long)(-0x80000000), (unsigned long)~a);
      check(78, (unsigned long)-(unsigned long)(-0x80000000), (unsigned long)-a);
      checkd(78, (double)(unsigned long)(-0x80000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x7fffffffffffffff); volatile long b = (long)(0xfffffffffffffff0);
      check(79, (unsigned long)((unsigned long)(0x7fffffffffffffff) * (long)(0xfffffffffffffff0)), (unsigned long)(a * b));
      check(79, (unsigned long)~(unsigned long)(0x7fffffffffffffff), (unsigned long)~a);
      check(79, (unsigned long)-(unsigned long)(0x7fffffffffffffff), (unsigned long)-a);
      checkd(79, (double)(unsigned long)(0x7fffffffffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(63); volatile int b = (int)(0x80000000);
      check(80, (unsigned long)((unsigned int)(63) ^ (int)(0x80000000)), (unsigned long)(a ^ b));
      check(80, (unsigned long)~(unsigned int)(63), (unsigned long)~a);
      check(80, (unsigned long)-(unsigned int)(63), (unsigned long)-a);
      checkd(80, (double)(unsigned int)(63), (double)a); }
    { volatile long a = (long)(0x100000000); volatile long b = (long)(0x7fffffff);
      check(81, (unsigned long)((long)(0x100000000) % (long)(0x7fffffff)), (unsigned long)(a % b));
      check(81, (unsigned long)~(long)(0x100000000), (unsigned long)~a);
      check(81, (unsigned long)-(long)(0x100000000), (unsigned long)-a);
      checkd(81, (double)(long)(0x100000000), (double)a); }
    { volatile long a = (long)(0); volatile int b = (int)(40);
      check(82, (unsigned long)((long)(0) >> (int)(40)), (unsigned long)(a >> b));
      check(82, (unsigned long)~(long)(0), (unsigned long)~a);
      check(82, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(82, (double)(long)(0), (double)a); }
    { volatile unsigned long a = (unsigned long)(63); volatile long b = (long)(31);
      check(83, (unsigned long)((unsigned long)(63) == (long)(31)), (unsigned long)(a == b));
      check(83, (unsigned long)~(unsigned long)(63), (unsigned long)~a);
      check(83, (unsigned long)-(unsigned long)(63), (unsigned long)-a);
      checkd(83, (double)(unsigned long)(63), (double)a); }
    { volatile int a = (int)(31); volatile unsigned int b = (unsigned int)(1);
      check(84, (unsigned long)((int)(31) % (unsigned int)(1)), (unsigned long)(a % b));
      check(84, (unsigned long)~(int)(31), (unsigned long)~a);
      check(84, (unsigned long)-(int)(31), (unsigned long)-a);
      checkd(84, (double)(int)(31), (double)a); }
    { volatile unsigned int a = (unsigned int)(63); volatile unsigned long b = (unsigned long)(63);
      check(85, (unsigned long)((unsigned int)(63) + (unsigned long)(63)), (unsigned long)(a + b));
      check(85, (unsigned long)~(unsigned int)(63), (unsigned long)~a);
      check(85, (unsigned long)-(unsigned int)(63), (unsigned long)-a);
      checkd(85, (double)(unsigned int)(63), (double)a); }
    { volatile unsigned long a = (unsigned long)(0); volatile long b = (long)(12345678901);
      check(86, (unsigned long)((unsigned long)(0) % (long)(12345678901)), (unsigned long)(a % b));
      check(86, (unsigned long)~(unsigned long)(0), (unsigned long)~a);
      check(86, (unsigned long)-(unsigned long)(0), (unsigned long)-a);
      checkd(86, (double)(unsigned long)(0), (double)a); }
    { volatile long a = (long)(-12345678901); volatile unsigned int b = (unsigned int)(1);
      check(87, (unsigned long)((long)(-12345678901) + (unsigned int)(1)), (unsigned long)(a + b));
      check(87, (unsigned long)~(long)(-12345678901), (unsigned long)~a);
      check(87, (unsigned long)-(long)(-12345678901), (unsigned long)-a);
      checkd(87, (double)(long)(-12345678901), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x100000000); volatile unsigned int b = (unsigned int)(0);
      check(88, (unsigned long)((unsigned long)(0x100000000) > (unsigned int)(0)), (unsigned long)(a > b));
      check(88, (unsigned long)~(unsigned long)(0x100000000), (unsigned long)~a);
      check(88, (unsigned long)-(unsigned long)(0x100000000), (unsigned long)-a);
      checkd(88, (double)(unsigned long)(0x100000000), (double)a); }
    { volatile long a = (long)(63); volatile long b = (long)(-1);
      check(89, (unsigned long)((long)(63) >= (long)(-1)), (unsigned long)(a >= b));
      check(89, (unsigned long)~(long)(63), (unsigned long)~a);
      check(89, (unsigned long)-(long)(63), (unsigned long)-a);
      checkd(89, (double)(long)(63), (double)a); }
    { volatile int a = (int)(0x100000000); volatile long b = (long)(0xffffffff);
      check(90, (unsigned long)((int)(0x100000000) | (long)(0xffffffff)), (unsigned long)(a | b));
      check(90, (unsigned long)~(int)(0x100000000), (unsigned long)~a);
      check(90, (unsigned long)-(int)(0x100000000), (unsigned long)-a);
      checkd(90, (double)(int)(0x100000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(40); volatile unsigned int b = (unsigned int)(0xfffffffffffffff0);
      check(91, (unsigned long)((unsigned long)(40) * (unsigned int)(0xfffffffffffffff0)), (unsigned long)(a * b));
      check(91, (unsigned long)~(unsigned long)(40), (unsigned long)~a);
      check(91, (unsigned long)-(unsigned long)(40), (unsigned long)-a);
      checkd(91, (double)(unsigned long)(40), (double)a); }
    { volatile unsigned int a = (unsigned int)(1); volatile int b = (int)(0x80000000);
      check(92, (unsigned long)((unsigned int)(1) * (int)(0x80000000)), (unsigned long)(a * b));
      check(92, (unsigned long)~(unsigned int)(1), (unsigned long)~a);
      check(92, (unsigned long)-(unsigned int)(1), (unsigned long)-a);
      checkd(92, (double)(unsigned int)(1), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x100000000); volatile int b = (int)(1);
      check(93, (unsigned long)((unsigned long)(0x100000000) % (int)(1)), (unsigned long)(a % b));
      check(93, (unsigned long)~(unsigned long)(0x100000000), (unsigned long)~a);
      check(93, (unsigned long)-(unsigned long)(0x100000000), (unsigned long)-a);
      checkd(93, (double)(unsigned long)(0x100000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(40); volatile long b = (long)(40);
      check(94, (unsigned long)((unsigned int)(40) / (long)(40)), (unsigned long)(a / b));
      check(94, (unsigned long)~(unsigned int)(40), (unsigned long)~a);
      check(94, (unsigned long)-(unsigned int)(40), (unsigned long)-a);
      checkd(94, (double)(unsigned int)(40), (double)a); }
    { volatile unsigned int a = (unsigned int)(63); volatile int b = (int)(-0x80000000);
      check(95, (unsigned long)((unsigned int)(63) > (int)(-0x80000000)), (unsigned long)(a > b));
      check(95, (unsigned long)~(unsigned int)(63), (unsigned long)~a);
      check(95, (unsigned long)-(unsigned int)(63), (unsigned long)-a);
      checkd(95, (double)(unsigned int)(63), (double)a); }
    { volatile unsigned int a = (unsigned int)(2); volatile int b = (int)(0);
      check(96, (unsigned long)((unsigned int)(2) >> (int)(0)), (unsigned long)(a >> b));
      check(96, (unsigned long)~(unsigned int)(2), (unsigned long)~a);
      check(96, (unsigned long)-(unsigned int)(2), (unsigned long)-a);
      checkd(96, (double)(unsigned int)(2), (double)a); }
    { volatile unsigned int a = (unsigned int)(-0x80000000); volatile long b = (long)(-12345678901);
      check(97, (unsigned long)((unsigned int)(-0x80000000) * (long)(-12345678901)), (unsigned long)(a * b));
      check(97, (unsigned long)~(unsigned int)(-0x80000000), (unsigned long)~a);
      check(97, (unsigned long)-(unsigned int)(-0x80000000), (unsigned long)-a);
      checkd(97, (double)(unsigned int)(-0x80000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xfffffffffffffff0); volatile int b = (int)(0x80000000);
      check(98, (unsigned long)((unsigned int)(0xfffffffffffffff0) | (int)(0x80000000)), (unsigned long)(a | b));
      check(98, (unsigned long)~(unsigned int)(0xfffffffffffffff0), (unsigned long)~a);
      check(98, (unsigned long)-(unsigned int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(98, (double)(unsigned int)(0xfffffffffffffff0), (double)a); }
    { volatile long a = (long)(31); volatile int b = (int)(3);
      check(99, (unsigned long)((long)(31) << (int)(3)), (unsigned long)(a << b));
      check(99, (unsigned long)~(long)(31), (unsigned long)~a);
      check(99, (unsigned long)-(long)(31), (unsigned long)-a);
      checkd(99, (double)(long)(31), (double)a); }
    { volatile unsigned long a = (unsigned long)(2); volatile int b = (int)(0x8000000000000000);
      check(100, (unsigned long)((unsigned long)(2) ^ (int)(0x8000000000000000)), (unsigned long)(a ^ b));
      check(100, (unsigned long)~(unsigned long)(2), (unsigned long)~a);
      check(100, (unsigned long)-(unsigned long)(2), (unsigned long)-a);
      checkd(100, (double)(unsigned long)(2), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xfffffffffffffff0); volatile unsigned int b = (unsigned int)(0);
      check(101, (unsigned long)((unsigned int)(0xfffffffffffffff0) & (unsigned int)(0)), (unsigned long)(a & b));
      check(101, (unsigned long)~(unsigned int)(0xfffffffffffffff0), (unsigned long)~a);
      check(101, (unsigned long)-(unsigned int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(101, (double)(unsigned int)(0xfffffffffffffff0), (double)a); }
    { volatile long a = (long)(-12345678901); volatile int b = (int)(40);
      check(102, (unsigned long)((long)(-12345678901) >> (int)(40)), (unsigned long)(a >> b));
      check(102, (unsigned long)~(long)(-12345678901), (unsigned long)~a);
      check(102, (unsigned long)-(long)(-12345678901), (unsigned long)-a);
      checkd(102, (double)(long)(-12345678901), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x8000000000000000); volatile unsigned int b = (unsigned int)(0xfffffffffffffff0);
      check(103, (unsigned long)((unsigned long)(0x8000000000000000) < (unsigned int)(0xfffffffffffffff0)), (unsigned long)(a < b));
      check(103, (unsigned long)~(unsigned long)(0x8000000000000000), (unsigned long)~a);
      check(103, (unsigned long)-(unsigned long)(0x8000000000000000), (unsigned long)-a);
      checkd(103, (double)(unsigned long)(0x8000000000000000), (double)a); }
    { volatile long a = (long)(0); volatile int b = (int)(0x7fffffffffffffff);
      check(104, (unsigned long)((long)(0) < (int)(0x7fffffffffffffff)), (unsigned long)(a < b));
      check(104, (unsigned long)~(long)(0), (unsigned long)~a);
      check(104, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(104, (double)(long)(0), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x80000000); volatile int b = (int)(3);
      check(105, (unsigned long)((unsigned int)(0x80000000) >> (int)(3)), (unsigned long)(a >> b));
      check(105, (unsigned long)~(unsigned int)(0x80000000), (unsigned long)~a);
      check(105, (unsigned long)-(unsigned int)(0x80000000), (unsigned long)-a);
      checkd(105, (double)(unsigned int)(0x80000000), (double)a); }
    { volatile int a = (int)(0xfffffffffffffff0); volatile long b = (long)(0xfffffffffffffff0);
      check(106, (unsigned long)((int)(0xfffffffffffffff0) * (long)(0xfffffffffffffff0)), (unsigned long)(a * b));
      check(106, (unsigned long)~(int)(0xfffffffffffffff0), (unsigned long)~a);
      check(106, (unsigned long)-(int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(106, (double)(int)(0xfffffffffffffff0), (double)a); }
    { volatile int a = (int)(0x100000000); volatile int b = (int)(0);
      check(107, (unsigned long)((int)(0x100000000) << (int)(0)), (unsigned long)(a << b));
      check(107, (unsigned long)~(int)(0x100000000), (unsigned long)~a);
      check(107, (unsigned long)-(int)(0x100000000), (unsigned long)-a);
      checkd(107, (double)(int)(0x100000000), (double)a); }
    { volatile long a = (long)(31); volatile int b = (int)(31);
      check(108, (unsigned long)((long)(31) << (int)(31)), (unsigned long)(a << b));
      check(108, (unsigned long)~(long)(31), (unsigned long)~a);
      check(108, (unsigned long)-(long)(31), (unsigned long)-a);
      checkd(108, (double)(long)(31), (double)a); }
    { volatile int a = (int)(0x8000000000000000); volatile unsigned long b = (unsigned long)(12345678901);
      check(109, (unsigned long)((int)(0x8000000000000000) + (unsigned long)(12345678901)), (unsigned long)(a + b));
      check(109, (unsigned long)~(int)(0x8000000000000000), (unsigned long)~a);
      check(109, (unsigned long)-(int)(0x8000000000000000), (unsigned long)-a);
      checkd(109, (double)(int)(0x8000000000000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(-1); volatile unsigned long b = (unsigned long)(1);
      check(110, (unsigned long)((unsigned int)(-1) != (unsigned long)(1)), (unsigned long)(a != b));
      check(110, (unsigned long)~(unsigned int)(-1), (unsigned long)~a);
      check(110, (unsigned long)-(unsigned int)(-1), (unsigned long)-a);
      checkd(110, (double)(unsigned int)(-1), (double)a); }
    { volatile unsigned int a = (unsigned int)(-0x80000000); volatile unsigned long b = (unsigned long)(40);
      check(111, (unsigned long)((unsigned int)(-0x80000000) - (unsigned long)(40)), (unsigned long)(a - b));
      check(111, (unsigned long)~(unsigned int)(-0x80000000), (unsigned long)~a);
      check(111, (unsigned long)-(unsigned int)(-0x80000000), (unsigned long)-a);
      checkd(111, (double)(unsigned int)(-0x80000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(40); volatile unsigned long b = (unsigned long)(12345678901);
      check(112, (unsigned long)((unsigned long)(40) < (unsigned long)(12345678901)), (unsigned long)(a < b));
      check(112, (unsigned long)~(unsigned long)(40), (unsigned long)~a);
      check(112, (unsigned long)-(unsigned long)(40), (unsigned long)-a);
      checkd(112, (double)(unsigned long)(40), (double)a); }
    { volatile int a = (int)(0x100000000); volatile int b = (int)(0x100000000);
      check(113, (unsigned long)((int)(0x100000000) == (int)(0x100000000)), (unsigned long)(a == b));
      check(113, (unsigned long)~(int)(0x100000000), (unsigned long)~a);
      check(113, (unsigned long)-(int)(0x100000000), (unsigned long)-a);
      checkd(113, (double)(int)(0x100000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(40); volatile int b = (int)(31);
      check(114, (unsigned long)((unsigned long)(40) / (int)(31)), (unsigned long)(a / b));
      check(114, (unsigned long)~(unsigned long)(40), (unsigned long)~a);
      check(114, (unsigned long)-(unsigned long)(40), (unsigned long)-a);
      checkd(114, (double)(unsigned long)(40), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x80000000); volatile long b = (long)(63);
      check(115, (unsigned long)((unsigned long)(0x80000000) >= (long)(63)), (unsigned long)(a >= b));
      check(115, (unsigned long)~(unsigned long)(0x80000000), (unsigned long)~a);
      check(115, (unsigned long)-(unsigned long)(0x80000000), (unsigned long)-a);
      checkd(115, (double)(unsigned long)(0x80000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(0x7fffffffffffffff); volatile unsigned int b = (unsigned int)(-12345678901);
      check(116, (unsigned long)((unsigned long)(0x7fffffffffffffff) != (unsigned int)(-12345678901)), (unsigned long)(a != b));
      check(116, (unsigned long)~(unsigned long)(0x7fffffffffffffff), (unsigned long)~a);
      check(116, (unsigned long)-(unsigned long)(0x7fffffffffffffff), (unsigned long)-a);
      checkd(116, (double)(unsigned long)(0x7fffffffffffffff), (double)a); }
    { volatile unsigned long a = (unsigned long)(0xffffffff); volatile unsigned long b = (unsigned long)(-1);
      check(117, (unsigned long)((unsigned long)(0xffffffff) & (unsigned long)(-1)), (unsigned long)(a & b));
      check(117, (unsigned long)~(unsigned long)(0xffffffff), (unsigned long)~a);
      check(117, (unsigned long)-(unsigned long)(0xffffffff), (unsigned long)-a);
      checkd(117, (double)(unsigned long)(0xffffffff), (double)a); }
    { volatile int a = (int)(0x7fffffffffffffff); volatile long b = (long)(0xffffffff);
      check(118, (unsigned long)((int)(0x7fffffffffffffff) <= (long)(0xffffffff)), (unsigned long)(a <= b));
      check(118, (unsigned long)~(int)(0x7fffffffffffffff), (unsigned long)~a);
      check(118, (unsigned long)-(int)(0x7fffffffffffffff), (unsigned long)-a);
      checkd(118, (double)(int)(0x7fffffffffffffff), (double)a); }
    { volatile int a = (int)(0); volatile unsigned long b = (unsigned long)(12345678901);
      check(119, (unsigned long)((int)(0) == (unsigned long)(12345678901)), (unsigned long)(a == b));
      check(119, (unsigned long)~(int)(0), (unsigned long)~a);
      check(119, (unsigned long)-(int)(0), (unsigned long)-a);
      checkd(119, (double)(int)(0), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xfffffffffffffff0); volatile unsigned long b = (unsigned long)(0x100000000);
      check(120, (unsigned long)((unsigned int)(0xfffffffffffffff0) < (unsigned long)(0x100000000)), (unsigned long)(a < b));
      check(120, (unsigned long)~(unsigned int)(0xfffffffffffffff0), (unsigned long)~a);
      check(120, (unsigned long)-(unsigned int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(120, (double)(unsigned int)(0xfffffffffffffff0), (double)a); }
    { volatile long a = (long)(0x100000000); volatile unsigned int b = (unsigned int)(40);
      check(121, (unsigned long)((long)(0x100000000) % (unsigned int)(40)), (unsigned long)(a % b));
      check(121, (unsigned long)~(long)(0x100000000), (unsigned long)~a);
      check(121, (unsigned long)-(long)(0x100000000), (unsigned long)-a);
      checkd(121, (double)(long)(0x100000000), (double)a); }
    { volatile long a = (long)(0xffffffff); volatile int b = (int)(0xfffffffffffffff0);
      check(122, (unsigned long)((long)(0xffffffff) == (int)(0xfffffffffffffff0)), (unsigned long)(a == b));
      check(122, (unsigned long)~(long)(0xffffffff), (unsigned long)~a);
      check(122, (unsigned long)-(long)(0xffffffff), (unsigned long)-a);
      checkd(122, (double)(long)(0xffffffff), (double)a); }
    { volatile unsigned int a = (unsigned int)(40); volatile unsigned int b = (unsigned int)(0);
      check(123, (unsigned long)((unsigned int)(40) * (unsigned int)(0)), (unsigned long)(a * b));
      check(123, (unsigned long)~(unsigned int)(40), (unsigned long)~a);
      check(123, (unsigned long)-(unsigned int)(40), (unsigned long)-a);
      checkd(123, (double)(unsigned int)(40), (double)a); }
    { volatile unsigned int a = (unsigned int)(-12345678901); volatile unsigned int b = (unsigned int)(40);
      check(124, (unsigned long)((unsigned int)(-12345678901) / (unsigned int)(40)), (unsigned long)(a / b));
      check(124, (unsigned long)~(unsigned int)(-12345678901), (unsigned long)~a);
      check(124, (unsigned long)-(unsigned int)(-12345678901), (unsigned long)-a);
      checkd(124, (double)(unsigned int)(-12345678901), (double)a); }
    { volatile unsigned long a = (unsigned long)(63); volatile unsigned long b = (unsigned long)(2);
      check(125, (unsigned long)((unsigned long)(63) > (unsigned long)(2)), (unsigned long)(a > b));
      check(125, (unsigned long)~(unsigned long)(63), (unsigned long)~a);
      check(125, (unsigned long)-(unsigned long)(63), (unsigned long)-a);
      checkd(125, (double)(unsigned long)(63), (double)a); }
    { volatile long a = (long)(0); volatile long b = (long)(31);
      check(126, (unsigned long)((long)(0) ^ (long)(31)), (unsigned long)(a ^ b));
      check(126, (unsigned long)~(long)(0), (unsigned long)~a);
      check(126, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(126, (double)(long)(0), (double)a); }
    { volatile long a = (long)(31); volatile int b = (int)(0x100000000);
      check(127, (unsigned long)((long)(31) != (int)(0x100000000)), (unsigned long)(a != b));
      check(127, (unsigned long)~(long)(31), (unsigned long)~a);
      check(127, (unsigned long)-(long)(31), (unsigned long)-a);
      checkd(127, (double)(long)(31), (double)a); }
    { volatile long a = (long)(-1); volatile long b = (long)(-0x80000000);
      check(128, (unsigned long)((long)(-1) | (long)(-0x80000000)), (unsigned long)(a | b));
      check(128, (unsigned long)~(long)(-1), (unsigned long)~a);
      check(128, (unsigned long)-(long)(-1), (unsigned long)-a);
      checkd(128, (double)(long)(-1), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xffffffff); volatile int b = (int)(0);
      check(129, (unsigned long)((unsigned int)(0xffffffff) + (int)(0)), (unsigned long)(a + b));
      check(129, (unsigned long)~(unsigned int)(0xffffffff), (unsigned long)~a);
      check(129, (unsigned long)-(unsigned int)(0xffffffff), (unsigned long)-a);
      checkd(129, (double)(unsigned int)(0xffffffff), (double)a); }
    { volatile int a = (int)(0x100000000); volatile unsigned int b = (unsigned int)(0x7fffffffffffffff);
      check(130, (unsigned long)((int)(0x100000000) ^ (unsigned int)(0x7fffffffffffffff)), (unsigned long)(a ^ b));
      check(130, (unsigned long)~(int)(0x100000000), (unsigned long)~a);
      check(130, (unsigned long)-(int)(0x100000000), (unsigned long)-a);
      checkd(130, (double)(int)(0x100000000), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xffffffff); volatile unsigned long b = (unsigned long)(0);
      check(131, (unsigned long)((unsigned int)(0xffffffff) != (unsigned long)(0)), (unsigned long)(a != b));
      check(131, (unsigned long)~(unsigned int)(0xffffffff), (unsigned long)~a);
      check(131, (unsigned long)-(unsigned int)(0xffffffff), (unsigned long)-a);
      checkd(131, (double)(unsigned int)(0xffffffff), (double)a); }
    { volatile int a = (int)(0); volatile long b = (long)(0x80000000);
      check(132, (unsigned long)((int)(0) >= (long)(0x80000000)), (unsigned long)(a >= b));
      check(132, (unsigned long)~(int)(0), (unsigned long)~a);
      check(132, (unsigned long)-(int)(0), (unsigned long)-a);
      checkd(132, (double)(int)(0), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x100000000); volatile long b = (long)(0xffffffff);
      check(133, (unsigned long)((unsigned int)(0x100000000) != (long)(0xffffffff)), (unsigned long)(a != b));
      check(133, (unsigned long)~(unsigned int)(0x100000000), (unsigned long)~a);
      check(133, (unsigned long)-(unsigned int)(0x100000000), (unsigned long)-a);
      checkd(133, (double)(unsigned int)(0x100000000), (double)a); }
    { volatile int a = (int)(40); volatile unsigned long b = (unsigned long)(1);
      check(134, (unsigned long)((int)(40) < (unsigned long)(1)), (unsigned long)(a < b));
      check(134, (unsigned long)~(int)(40), (unsigned long)~a);
      check(134, (unsigned long)-(int)(40), (unsigned long)-a);
      checkd(134, (double)(int)(40), (double)a); }
    { volatile unsigned int a = (unsigned int)(0xfffffffffffffff0); volatile int b = (int)(0x80000000);
      check(135, (unsigned long)((unsigned int)(0xfffffffffffffff0) + (int)(0x80000000)), (unsigned long)(a + b));
      check(135, (unsigned long)~(unsigned int)(0xfffffffffffffff0), (unsigned long)~a);
      check(135, (unsigned long)-(unsigned int)(0xfffffffffffffff0), (unsigned long)-a);
      checkd(135, (double)(unsigned int)(0xfffffffffffffff0), (double)a); }
    { volatile int a = (int)(0x80000000); volatile long b = (long)(40);
      check(136, (unsigned long)((int)(0x80000000) | (long)(40)), (unsigned long)(a | b));
      check(136, (unsigned long)~(int)(0x80000000), (unsigned long)~a);
      check(136, (unsigned long)-(int)(0x80000000), (unsigned long)-a);
      checkd(136, (double)(int)(0x80000000), (double)a); }
    { volatile int a = (int)(0xffffffff); volatile unsigned long b = (unsigned long)(-12345678901);
      check(137, (unsigned long)((int)(0xffffffff) ^ (unsigned long)(-12345678901)), (unsigned long)(a ^ b));
      check(137, (unsigned long)~(int)(0xffffffff), (unsigned long)~a);
      check(137, (unsigned long)-(int)(0xffffffff), (unsigned long)-a);
      checkd(137, (double)(int)(0xffffffff), (double)a); }
    { volatile int a = (int)(2); volatile int b = (int)(40);
      check(138, (unsigned long)((int)(2) & (int)(40)), (unsigned long)(a & b));
      check(138, (unsigned long)~(int)(2), (unsigned long)~a);
      check(138, (unsigned long)-(int)(2), (unsigned long)-a);
      checkd(138, (double)(int)(2), (double)a); }
    { volatile unsigned long a = (unsigned long)(12345678901); volatile unsigned int b = (unsigned int)(0x7fffffff);
      check(139, (unsigned long)((unsigned long)(12345678901) % (unsigned int)(0x7fffffff)), (unsigned long)(a % b));
      check(139, (unsigned long)~(unsigned long)(12345678901), (unsigned long)~a);
      check(139, (unsigned long)-(unsigned long)(12345678901), (unsigned long)-a);
      checkd(139, (double)(unsigned long)(12345678901), (double)a); }
    { volatile long a = (long)(0); volatile unsigned long b = (unsigned long)(31);
      check(140, (unsigned long)((long)(0) != (unsigned long)(31)), (unsigned long)(a != b));
      check(140, (unsigned long)~(long)(0), (unsigned long)~a);
      check(140, (unsigned long)-(long)(0), (unsigned long)-a);
      checkd(140, (double)(long)(0), (double)a); }
    { volatile long a = (long)(0x7fffffff); volatile long b = (long)(0xfffffffffffffff0);
      check(141, (unsigned long)((long)(0x7fffffff) > (long)(0xfffffffffffffff0)), (unsigned long)(a > b));
      check(141, (unsigned long)~(long)(0x7fffffff), (unsigned long)~a);
      check(141, (unsigned long)-(long)(0x7fffffff), (unsigned long)-a);
      checkd(141, (double)(long)(0x7fffffff), (double)a); }
    { volatile int a = (int)(-1); volatile long b = (long)(0x7fffffff);
      check(142, (unsigned long)((int)(-1) < (long)(0x7fffffff)), (unsigned long)(a < b));
      check(142, (unsigned long)~(int)(-1), (unsigned long)~a);
      check(142, (unsigned long)-(int)(-1), (unsigned long)-a);
      checkd(142, (double)(int)(-1), (double)a); }
    { volatile unsigned long a = (unsigned long)(63); volatile unsigned long b = (unsigned long)(-12345678901);
      check(143, (unsigned long)((unsigned long)(63) - (unsigned long)(-12345678901)), (unsigned long)(a - b));
      check(143, (unsigned long)~(unsigned long)(63), (unsigned long)~a);
      check(143, (unsigned long)-(unsigned long)(63), (unsigned long)-a);
      checkd(143, (double)(unsigned long)(63), (double)a); }
    { volatile int a = (int)(0x8000000000000000); volatile unsigned int b = (unsigned int)(0x7fffffffffffffff);
      check(144, (unsigned long)((int)(0x8000000000000000) > (unsigned int)(0x7fffffffffffffff)), (unsigned long)(a > b));
      check(144, (unsigned long)~(int)(0x8000000000000000), (unsigned long)~a);
      check(144, (unsigned long)-(int)(0x8000000000000000), (unsigned long)-a);
      checkd(144, (double)(int)(0x8000000000000000), (double)a); }
    { volatile unsigned long a = (unsigned long)(0); volatile int b = (int)(0);
      check(145, (unsigned long)((unsigned long)(0) << (int)(0)), (unsigned long)(a << b));
      check(145, (unsigned long)~(unsigned long)(0), (unsigned long)~a);
      check(145, (unsigned long)-(unsigned long)(0), (unsigned long)-a);
      checkd(145, (double)(unsigned long)(0), (double)a); }
    { volatile int a = (int)(2); volatile unsigned int b = (unsigned int)(0x80000000);
      check(146, (unsigned long)((int)(2) == (unsigned int)(0x80000000)), (unsigned long)(a == b));
      check(146, (unsigned long)~(int)(2), (unsigned long)~a);
      check(146, (unsigned long)-(int)(2), (unsigned long)-a);
      checkd(146, (double)(int)(2), (double)a); }
    { volatile int a = (int)(12345678901); volatile int b = (int)(-1);
      check(147, (unsigned long)((int)(12345678901) - (int)(-1)), (unsigned long)(a - b));
      check(147, (unsigned long)~(int)(12345678901), (unsigned long)~a);
      check(147, (unsigned long)-(int)(12345678901), (unsigned long)-a);
      checkd(147, (double)(int)(12345678901), (double)a); }
    { volatile unsigned int a = (unsigned int)(0x8000000000000000); volatile unsigned long b = (unsigned long)(-12345678901);
      check(148, (unsigned long)((unsigned int)(0x8000000000000000) | (unsigned long)(-12345678901)), (unsigned long)(a | b));
      check(148, (unsigned long)~(unsigned int)(0x8000000000000000), (unsigned long)~a);
      check(148, (unsigned long)-(unsigned int)(0x8000000000000000), (unsigned long)-a);
      checkd(148, (double)(unsigned int)(0x8000000000000000), (double)a); }
    { volatile int a = (int)(40); volatile int b = (int)(0);
      check(149, (unsigned long)((int)(40) != (int)(0)), (unsigned long)(a != b));
      check(149, (unsigned long)~(int)(40), (unsigned long)~a);
      check(149, (unsigned long)-(int)(40), (unsigned long)-a);
      checkd(149, (double)(int)(40), (double)a); }
    printf("%d bad\n", bad);
    return 0;
}

// CHECK: 0 bad
