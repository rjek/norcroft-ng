// Folding ~ and ^ on unsigned long constants.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
typedef long lua_Integer;
typedef unsigned long lua_Unsigned;
#define l_castS2U(i) ((lua_Unsigned)(i))
#define l_castU2S(i) ((lua_Integer)(i))
#define intop(op,v1,v2) l_castU2S(l_castS2U(v1) op l_castS2U(v2))
lua_Integer bnot(lua_Integer v1) { return intop(^, ~l_castS2U(0), v1); }
lua_Integer bnot2(lua_Integer v1) { return ~v1; }
unsigned long un(void) { return ~l_castS2U(0); }
int main(void)
{   volatile long z = 0, f = 0xf0;
    printf("%ld %ld %ld %lu %lx\n", bnot(z), bnot(f), bnot2(z), un(), ~0UL);
    printf("%ld %ld\n", intop(^, ~l_castS2U(0), 0), (long)~(unsigned long)0);
    return 0;
}

// CHECK: -1 -241 -1 18446744073709551615 ffffffffffffffff
// CHECK: -1 -1
