// Position-independent code in a shared library: data and functions
// reached via the GOT and PLT, pointers in data, and a switch table.
// RUN: %cc -DLIB -c %s -o %t.lib.o && cc -shared -Wl,-z,text %t.lib.o -o %t.so && %cc -c %s -o %t.o && cc -Wl,-z,text %t.o %t.so -o %t && %t
// REQUIRES: x86_64, x86_64-run

#ifdef LIB
int counter = 10;
static const char *msg[] = { "lo", "hi" };
static int bump(int n) { return counter += n; }
int (*hook)(int) = bump;
const char *libfn(int x)
{   hook(x);
    switch (x) {
    case 1: return msg[0]; case 2: return msg[1]; case 3: return "three";
    case 4: return "four"; default: return "other";
    }
}
#else
int printf(const char *, ...);
extern int counter;
extern const char *libfn(int);
int main(void)
{   printf("%s %s %s %s %d\n", libfn(1), libfn(2), libfn(4), libfn(9), counter);
    return 0;
}
#endif

// CHECK: lo hi four other 26
