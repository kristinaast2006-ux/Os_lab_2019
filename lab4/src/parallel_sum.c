#define _GNU_SOURCE

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sum_lib.h"

struct ThreadArg {
  struct SumArgs args;
  long long result;
};

static void GenerateArray(int *array, int array_size, unsigned int seed) {
  srand(seed);
  for (int i = 0; i < array_size; ++i) {
    array[i] = rand() % 100;
  }
}

static int parse_long_arg(const char *s, long min, long max, long *out) {
  char *end = NULL;
  errno = 0;
  long v = strtol(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0' || v < min || v > max) {
    return -1;
  }
  *out = v;
  return 0;
}

static void *ThreadSum(void *arg) {
  struct ThreadArg *thread_arg = (struct ThreadArg *)arg;
  thread_arg->result = Sum(&thread_arg->args);
  return NULL;
}

static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s --threads_num N --seed S --array_size N\n",
          prog);
}

int main(int argc, char **argv) {
  int threads_num = 1;
  int array_size = 1000;
  unsigned int seed = 0;

  static struct option long_options[] = {
      {"threads_num", required_argument, 0, 't'},
      {"seed", required_argument, 0, 's'},
      {"array_size", required_argument, 0, 'a'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}};

  int opt;
  long v;

  while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
    switch (opt) {
      case 't':
        if (parse_long_arg(optarg, 1, 1000000, &v) < 0) {
          usage(argv[0]);
          return 1;
        }
        threads_num = (int)v;
        break;
      case 's':
        if (parse_long_arg(optarg, 0, 2147483647L, &v) < 0) {
          usage(argv[0]);
          return 1;
        }
        seed = (unsigned int)v;
        break;
      case 'a':
        if (parse_long_arg(optarg, 1, 2147483647L, &v) < 0) {
          usage(argv[0]);
          return 1;
        }
        array_size = (int)v;
        break;
      case 'h':
        usage(argv[0]);
        return 0;
      default:
        usage(argv[0]);
        return 1;
    }
  }

  int *array = malloc(sizeof(int) * (size_t)array_size);
  if (array == NULL) {
    perror("malloc");
    return 1;
  }

  GenerateArray(array, array_size, seed);

  struct ThreadArg *args = malloc(sizeof(*args) * (size_t)threads_num);
  pthread_t *threads = malloc(sizeof(*threads) * (size_t)threads_num);

  if (args == NULL || threads == NULL) {
    perror("malloc");
    free(array);
    free(args);
    free(threads);
    return 1;
  }

  for (int i = 0; i < threads_num; ++i) {
    args[i].args.array = array;
    args[i].args.begin =
        (int)((long long)i * array_size / threads_num);
    args[i].args.end =
        (int)((long long)(i + 1) * array_size / threads_num);
    args[i].result = 0;
  }

  struct timespec start;
  struct timespec end;

  clock_gettime(CLOCK_MONOTONIC, &start);

  for (int i = 0; i < threads_num; ++i) {
    if (pthread_create(&threads[i], NULL, ThreadSum, &args[i]) != 0) {
      fprintf(stderr, "Error: pthread_create failed!\n");
      free(array);
      free(args);
      free(threads);
      return 1;
    }
  }

  long long total_sum = 0;

  for (int i = 0; i < threads_num; ++i) {
    pthread_join(threads[i], NULL);
    total_sum += args[i].result;
  }

  clock_gettime(CLOCK_MONOTONIC, &end);

  double elapsed =
      (double)(end.tv_sec - start.tv_sec) +
      (double)(end.tv_nsec - start.tv_nsec) / 1e9;

  printf("Total: %lld\n", total_sum);
  printf("Time: %.6f s\n", elapsed);

  free(array);
  free(args);
  free(threads);

  return 0;
}