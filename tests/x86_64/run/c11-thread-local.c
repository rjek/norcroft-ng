// C11's _Thread_local, external, static and static in a function (which
// may be inlined), in threads of their own.
// RUN: %cc -std=c11 %s -o %t -lpthread && %t | sort
// REQUIRES: x86_64, x86_64-run

#include <stdio.h>
#include <pthread.h>
_Thread_local int counter = 5;
static _Thread_local long big[4] = { 1, 2, 3, 4 };
_Thread_local int zeroed;
int bump(void) { static _Thread_local int calls; return ++calls; }
void *work(void *arg)
{   int id = (int)(long)arg, i;
    for (i = 0; i < 1000 * (id + 1); i++) { counter++; big[2] += 2; zeroed--; bump(); }
    printf("thread %d: %d %ld %d %d\n", id, counter, big[2], zeroed, bump() - 1);
    return 0;
}
int main(void)
{   pthread_t t[3];
    int i;
    for (i = 0; i < 3; i++) pthread_create(&t[i], 0, work, (void *)(long)i);
    for (i = 0; i < 3; i++) pthread_join(t[i], 0);
    printf("main: %d %ld %d %d\n", counter, big[2], zeroed, bump());
    return 0;
}

// CHECK: main: 5 3 0 1
// CHECK: thread 0: 1005 2003 -1000 1000
// CHECK: thread 1: 2005 4003 -2000 2000
// CHECK: thread 2: 3005 6003 -3000 3000
