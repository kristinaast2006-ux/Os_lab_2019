#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>  
#include <time.h>    

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"


static volatile sig_atomic_t timeout_expired = 0;
static void handle_alarm(int sig) {
  (void)sig;
  timeout_expired = 1;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) { printf("seed must be positive\n"); return 1; }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) { printf("array_size must be positive\n"); return 1; }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) { printf("pnum must be positive\n"); return 1; }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) { printf("timeout must be positive\n"); return 1; }
            break;
          case 4:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) { printf("Has at least one no option argument\n"); return 1; }
  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  int active_child_processes = 0;
  pid_t *child_pids = malloc(sizeof(pid_t) * pnum);

  int (*pipes)[2] = NULL;
  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) { perror("pipe"); return 1; }
    }
  }

  if (timeout > 0) {
    signal(SIGALRM, handle_alarm);
    alarm(timeout);
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int chunk = array_size / pnum;
  int remainder = array_size % pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[i] = child_pid;
      
      if (child_pid == 0) {
        unsigned int begin = i * chunk + (i < remainder ? i : remainder);
        unsigned int end = begin + chunk + (i < remainder ? 1 : 0);
        struct MinMax mm = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          snprintf(filename, sizeof(filename), "child_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (!f) { perror("fopen"); return 1; }
          fprintf(f, "%d %d\n", mm.min, mm.max);
          fclose(f);
        } else {
          for (int j = 0; j < pnum; j++) {
            close(pipes[j][0]);
            if (j != i) close(pipes[j][1]);
          }
          int data[2] = {mm.min, mm.max};
          if (write(pipes[i][1], data, sizeof(data)) != sizeof(data)) {
            perror("write"); return 1;
          }
          close(pipes[i][1]);
        }
        free(array);
        exit(0);
      }
    } else {
      printf("Fork failed!\n"); return 1;
    }
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) close(pipes[i][1]);
  }

  int reaped = 0;
  while (reaped < active_child_processes) {
    int status;
    pid_t w = waitpid(-1, &status, WNOHANG);
    
    if (w > 0) {
      reaped++;
    } else if (w == 0) {
      if (timeout_expired) {
        for (int i = 0; i < pnum; i++) {
          kill(child_pids[i], SIGKILL);
        }
        while (waitpid(-1, &status, 0) > 0) {}
        break;
      }
      struct timespec ts = {0, 10000000L}; // 10 мс
      nanosleep(&ts, NULL);
    } else {
      if (errno == EINTR) continue;
      break;
    }
  }

  if (timeout > 0) alarm(0);

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;
  bool found_any = false;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "child_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (!f) { continue; } 
      if (fscanf(f, "%d %d", &min, &max) == 2) {
         found_any = true;
      }
      fclose(f);
      remove(filename);
    } else {
      int data[2];
      if (read(pipes[i][0], data, sizeof(data)) == sizeof(data)) {
        min = data[0];
        max = data[1];
        found_any = true;
      }
      close(pipes[i][0]);
    }

    if (found_any) {
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);
  if (!with_files) free(pipes);

  if (found_any) {
      printf("Min: %d\n", min_max.min);
      printf("Max: %d\n", min_max.max);
  } else {
      printf("Timeout: no results\n");
  }
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
