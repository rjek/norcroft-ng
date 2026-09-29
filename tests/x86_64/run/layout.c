// Struct and union sizes and member alignment.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
struct c1 { char c; };
struct c3 { char c[3]; };
struct s1 { char c; short s; };
struct s2 { short s; char c; };
struct i1 { char c; int i; char d; };
struct d1 { char c; double d; };
struct n1 { struct c3 a; char b; struct c1 c; };
union u1 { char c[5]; short s; };
struct c3 arr[4];
int main(void)
{   printf("%d %d %d %d %d %d %d %d %d\n", (int)sizeof(struct c1), (int)sizeof(struct c3),
           (int)sizeof(struct s1), (int)sizeof(struct s2), (int)sizeof(struct i1),
           (int)sizeof(struct n1), (int)sizeof(union u1), (int)sizeof(arr),
           (int)((char *)&arr[2] - (char *)&arr[0]));
    return 0;
}

// CHECK: 1 3 4 4 12 5 6 12 6
