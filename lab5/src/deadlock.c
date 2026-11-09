#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg) {
    (void)arg;

    pthread_mutex_lock(&m1);
    printf("thread1: locked m1\n");
    fflush(stdout);

    sleep(1);

    printf("thread1: trying to lock m2\n");
    fflush(stdout);

    pthread_mutex_lock(&m2);
    printf("thread1: locked m2\n");

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);

    return NULL;
}

void *thread2(void *arg) {
    (void)arg;

    pthread_mutex_lock(&m2);
    printf("thread2: locked m2\n");
    fflush(stdout);

    sleep(1);

    printf("thread2: trying to lock m1\n");
    fflush(stdout);

    pthread_mutex_lock(&m1);
    printf("thread2: locked m1\n");

    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);

    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}