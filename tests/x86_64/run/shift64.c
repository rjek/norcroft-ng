// 64-bit shifts by variable amounts.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
typedef long lua_Integer;
typedef unsigned long lua_Unsigned;
#define NBITS ((int)(sizeof(lua_Integer) * 8))
#define intop(op,v1,v2) ((lua_Integer)((lua_Unsigned)(v1) op (lua_Unsigned)(v2)))
lua_Integer shiftl(lua_Integer x, lua_Integer y) {
  if (y < 0) { if (y <= -NBITS) return 0; else return intop(>>, x, -y); }
  else { if (y >= NBITS) return 0; else return intop(<<, x, y); }
}
int main(void)
{   volatile long a = 1, b = 40, c = -3, d = 70;
    printf("%ld %ld %ld %ld\n", shiftl(a, b), shiftl(1L << 50, c), shiftl(a, d), shiftl(-1, -60));
    printf("%ld %ld\n", (long)((unsigned long)a << b), a << b);
    return 0;
}

// CHECK: 1099511627776 140737488355328 0 15
// CHECK: 1099511627776 1099511627776
