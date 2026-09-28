// A conditional expression lifted out of a loop (a CSE regression).
// RUN: %cc %s -o %t && %t
// REQUIRES: i386, i386-run

int printf(const char *, ...);
typedef unsigned char bool;
static int curchar;
static char buf[64];
static bool put(char *where, int size, bool escaped)
{   int ch = curchar;
    if (escaped) switch (ch) {
    case 'a': ch = 7; break;  case 'b': ch = 8; break;  case 'f': ch = 12; break;
    case 'n': ch = 10; break; case 'r': ch = 13; break; case 't': ch = 9; break;
    case 'v': ch = 11; break; case '\\': ch = '\\'; break; case '0': ch = 0; break;
    default: return 0;
    }
    if (size == 4) *(int *)where = ch; else *where = (char)ch;
    return 1;
}
static int lex(int type, const char *s)
{   char *val = buf, *p = buf;
    for (; *s; s++) {
        bool escaped = *s == '\\';
        if (escaped) s++;
        curchar = *s;
        if (put(p, (type == 3 ? 4 : 1), escaped))
            p += (type == 3 ? 4 : 1);
    }
    return p - val;
}
int main(void)
{   printf("%d %d %d %d\n", lex(3, "\\0"), lex(1, "\\0"), lex(3, "ab\\n"), lex(1, "x\\qy"));
    printf("%d %d %d\n", buf[0], buf[4], buf[8]);
    return 0;
}

// CHECK: 4 1 12 2
// CHECK: 0 98 10
