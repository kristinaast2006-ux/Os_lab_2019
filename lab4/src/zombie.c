#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    printf("Child PID=%d exits now\n", getpid());
    _exit(0);
  }

  printf("Parent PID=%d, child PID=%d\n", getpid(), pid);
  printf("Child is zombie until parent calls waitpid(). Sleeping 10 sec...\n");

  sleep(10);

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "ps -o pid,ppid,stat,cmd -p %d", pid);
  system(cmd);

  waitpid(pid, NULL, 0);
  printf("Parent called waitpid(), zombie removed.\n");

  return 0;
}