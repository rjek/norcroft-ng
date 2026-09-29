// char and short loads, stores and extension; plain char is signed.
// RUN: %cc %s -o %t && %t
// REQUIRES: i386, i386-run

int printf(const char *, ...);
signed char sc[4] = { -1, 127, -128, 5 };
unsigned char uc[4] = { 255, 127, 128, 5 };
short ss[3] = { -1, 32767, -32768 };
unsigned short us[3] = { 65535, 32767, 32768 };
char plain = -1;
void store(signed char *p, int v, int w, int x) { p[0] = v; p[1] = w; p[2] = x; p[3] = v + w + x; }
int sum(signed char *p, int n) { int s = 0; while (n--) s += *p++; return s; }
int main(void)
{   signed char buf[4]; short sh; int i;
    for (i = 0; i < 4; i++) printf("%d %d\n", sc[i], uc[i]);
    for (i = 0; i < 3; i++) printf("%d %d\n", ss[i], us[i]);
    printf("plain char is %s\n", plain < 0 ? "signed" : "unsigned");
    store(buf, 1000, -3, 77);
    printf("%d %d %d %d sum %d\n", buf[0], buf[1], buf[2], buf[3], sum(buf, 4));
    sh = 70000; printf("%d\n", sh);
    return 0;
}

// CHECK: -1 255
// CHECK: 127 127
// CHECK: -128 128
// CHECK: 5 5
// CHECK: -1 65535
// CHECK: 32767 32767
// CHECK: -32768 32768
// CHECK: plain char is signed
// CHECK: -24 -3 77 50 sum 100
// CHECK: 4464
