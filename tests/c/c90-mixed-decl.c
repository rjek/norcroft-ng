// Declarations after statements are an error in C90 (the default).
// RUN: %cc %s -c -o %t.o
// EXPECT-ERROR
// CHECK-ERR: <command> expected but found 'int'

int f(int n)
{   n++;
    int m = n;
    return m;
}
