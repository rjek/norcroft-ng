// Switch statements, including a dense case table.
// RUN: %cc %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

int printf(const char *, ...);
const char *name(int n)
{   switch (n) {
    case 0: return "zero"; case 1: return "one"; case 2: return "two";
    case 3: return "three"; case 4: return "four"; case 5: return "five";
    case 7: return "seven"; case 100: return "hundred";
    default: return "other";
    }
}
int main(void) { int i; for (i = -2; i < 10; i++) printf("%d %s\n", i, name(i)); printf("%s\n", name(100)); return 0; }

// CHECK: -2 other
// CHECK: -1 other
// CHECK: 0 zero
// CHECK: 1 one
// CHECK: 2 two
// CHECK: 3 three
// CHECK: 4 four
// CHECK: 5 five
// CHECK: 6 other
// CHECK: 7 seven
// CHECK: 8 other
// CHECK: 9 other
// CHECK: hundred
