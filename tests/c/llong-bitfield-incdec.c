// Postfix ++ and -- of long long bitfields (which crashed the compiler
// where long long is a pair of words).
// RUN: %cc %s -c -o %t.o

struct s { int i; long long f : 40; unsigned long long g : 12; long long h : 3; } x;
struct s *next(void);

long long f(struct s *p)
{   long long r = x.f++ + x.g-- + x.h++;
    r += p->f--;
    r += next()->g++;
    x.f++;
    return r;
}
