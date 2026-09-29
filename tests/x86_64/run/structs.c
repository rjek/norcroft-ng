// Struct arguments and results (in registers, or via a hidden pointer) and struct copies.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
struct pt { int x, y; };
struct big { int a[10]; char c; };
struct pt mk(int x, int y) { struct pt p; p.x = x; p.y = y; return p; }
struct big mkbig(int k) { struct big b; int i; for (i = 0; i < 10; i++) b.a[i] = k * i; b.c = 'q'; return b; }
int dot(struct pt a, struct pt b) { return a.x * b.x + a.y * b.y; }
int total(struct big b) { int i, s = 0; for (i = 0; i < 10; i++) s += b.a[i]; return s + b.c; }
struct big gb;
int main(void)
{   struct pt p = mk(3, 4), q = mk(5, -6);
    struct big b = mkbig(3), c;
    printf("%d %d %d\n", p.x, p.y, dot(p, q));
    c = b; gb = c;
    printf("%d %d %c\n", total(b), total(gb), gb.c);
    c = mkbig(2); printf("%d\n", c.a[9]);
    return 0;
}

// CHECK: 3 4 -9
// CHECK: 248 248 q
// CHECK: 18
