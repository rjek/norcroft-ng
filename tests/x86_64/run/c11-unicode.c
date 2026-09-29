// C11's u"", U"" and u8"" strings and u'' and U'' characters (with
// UTF-16 surrogates, and concatenation with ordinary strings).
// RUN: %cc -std=c11 %s -o %t && %t
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <uchar.h>
static const char16_t s16[] = u"hé\U0001F600";
static const char32_t s32[] = U"hé\U0001F600" "x";
static char16_t c16 = u'€';
int main(void)
{   const char *u8 = u8"hé";
    const char16_t *p16 = u"a" "é" u"b";
    int i;
    for (i = 0; s16[i]; i++) printf("%x ", s16[i]);
    printf("|");
    for (i = 0; s32[i]; i++) printf(" %x", (unsigned)s32[i]);
    printf(" | ");
    for (i = 0; u8[i]; i++) printf("%02x", (unsigned char)u8[i]);
    printf(" |");
    for (i = 0; p16[i]; i++) printf(" %x", p16[i]);
    printf(" | %x %zu %zu %zu %zu %d %d\n", c16, sizeof(u"ab"), sizeof(U"ab"),
           sizeof s16, sizeof(u'x'), __STDC_UTF_16__, __STDC_UTF_32__);
    return 0;
}

// CHECK: 68 e9 d83d de00 | 68 e9 1f600 78 | 68c3a9 | 61 e9 62 | 20ac 6 12 10 2 1 1
