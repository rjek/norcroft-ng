// #if's arithmetic is in intmax_t and uintmax_t from C99.
// RUN: %cc -std=c99 %s -c -o %t.o

#if 1 << 40 != 0x10000000000
#error shift
#endif
#if 2147483647 + 1 < 0
#error int overflow
#endif
#if -1 > 0u
#else
#error uintmax
#endif
#if 0xffffffffffffffff != -1
#error ull
#endif
#if (0x7fffffffffffffff + 0) / 3 != 3074457345618258602
#error divide
#endif
#if 0x100000000
#else
#error nonzero
#endif
#if (1 ? 0x100000000 : 0) <= 5 || (defined X ? 1 : 0) >= 5
#error conditional
#endif
#if 'a' * 0x100000000 != 0x6100000000
#error char
#endif
#if defined(__STDC__) + 0x100000000 != 0x100000001
#error defined
#endif
int ok;
