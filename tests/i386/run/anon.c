// C11 anonymous struct and union members.
// RUN: %cc %s -o %t && %t
// REQUIRES: i386, i386-run

int printf(const char *, ...);
struct s {
    int a;
    union { int i; float f; struct { short lo, hi; }; };
    struct { int c; int d : 5; int e : 3; };
    int z;
};
int main(void)
{   struct s x, *p = &x;
    x.a = 1; x.i = 0x00020001; x.c = 'k'; x.d = -3; x.e = 2; x.z = 9;
    printf("%d %d %d %d %c %d %d %d\n", x.a, x.i, p->lo, p->hi, p->c, x.d, x.e, p->z);
    printf("%d\n", (int)sizeof(struct s));
    return 0;
}

// CHECK: 1 131073 1 2 k -3 2 9
// CHECK: 20
