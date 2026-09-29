// Errors in C23's _BitInt types and literals.
// RUN: %cc -std=c23 %s -c -o %t.o
// EXPECT-ERROR
// CHECK-ERR: Error: _BitInt width 0 is not in the range 1 (unsigned) or 2 to 64
// CHECK-ERR: Error: _BitInt width 65 is not in the range 1 (unsigned) or 2 to 64
// CHECK-ERR: Error: _BitInt width 1 is not in the range 1 (unsigned) or 2 to 64
_BitInt(0) a;
_BitInt(65) b;
_BitInt(1) c;
unsigned _BitInt(1) d;
// CHECK-ERR: Serious error: type '_BitInt' inconsistent with 'int'
int _BitInt(8) e;
// CHECK-ERR: Error: repeated _BitInt
_BitInt(3) _BitInt(4) g;
int n = 3;
// CHECK-ERR: Error: _BitInt width must be an integer constant
_BitInt(n) h;
// CHECK-ERR: Serious error: Number 8000000000000000 too large for 64-bit implementation
long long z = 0x8000000000000000wb;
