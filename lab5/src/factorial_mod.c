#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    long long k;
    int pnum;
    long long mod;
    long long partial;
} ThreadArg;

pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
long long result = 1;

static long long mul_mod(long long a, long long b, long long m) {
    return (long long)(((__int128)a * b) % m);
}

void *worker(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;

    long long partial = 1 % a->mod;

    for (long long i = a->id + 1; i <= a->k; i += a->pnum) {
        partial = mul_mod(partial, i % a->mod, a->mod);
    }

    a->partial = partial;

    pthread_mutex_lock(&result_mutex);
    result = mul_mod(result, partial, a->mod);
    pthread_mutex_unlock(&result_mutex);

    return NULL;
}

int main(int argc, char **argv) {
    long long k = -1;
    long long mod = -1;
    int pnum = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = strtoll(argv[++i], NULL, 10);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = strtoll(argv[i] + 6, NULL, 10);
        } else {
            fprintf(stderr, "Неизвестный аргумент: %s\n", argv[i]);
            return 1;
        }
    }

    if (k < 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Использование: %s -k K --pnum=N --mod=M\n", argv[0]);
        return 1;
    }

    pthread_t *threads = calloc(pnum, sizeof(*threads));
    ThreadArg *args = calloc(pnum, sizeof(*args));

    if (threads == NULL || args == NULL) {
        perror("calloc");
        return 1;
    }

    for (int i = 0; i < pnum; i++) {
        args[i].id = i;
        args[i].k = k;
        args[i].pnum = pnum;
        args[i].mod = mod;
        args[i].partial = 1;

        if (pthread_create(&threads[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }

    printf("%lld! mod %lld = %lld\n", k, mod, result);

    free(threads);
    free(args);

    return 0;
}