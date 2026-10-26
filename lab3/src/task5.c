#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s seed array_size\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    // child: replace image with sequential_min_max
    execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], (char *)NULL);
    // if execl returns — error
    perror("execl");
    exit(1);
  }

  int status;
  if (waitpid(pid, &status, 0) == -1) {
    perror("waitpid");
    return 1;
  }

  if (WIFEXITED(status)) {
    printf("child exited with code %d\n", WEXITSTATUS(status));
    return WEXITSTATUS(status);
  } else {
    printf("child terminated abnormally\n");
    return 1;
  }
}