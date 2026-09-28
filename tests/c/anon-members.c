// C11 anonymous struct and union members.
// RUN: %cc %s -c

#include <stddef.h>

struct s {
    int a;
    union { int i; float f; struct { short lo, hi; }; };
    struct { int c; int d; };
    int z;
};

_Static_assert(offsetof(struct s, i) == 4, "i");
_Static_assert(offsetof(struct s, f) == 4, "f");
_Static_assert(offsetof(struct s, hi) == 6, "hi");
_Static_assert(offsetof(struct s, d) == 12, "d");
_Static_assert(offsetof(struct s, z) == 16, "z");

int get(struct s *p) { return p->a + p->i + p->hi + p->d + p->z; }
