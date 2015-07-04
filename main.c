#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CMDS 16
#define SEP      "---"

typedef pid_t pids_t[MAX_CMDS];

void wait_for_exit(int n_cmds, pids_t *pids) {
  int cmds_left = n_cmds;

  for (;;) {
    int status;
    pid_t waited_pid = waitpid(-1, &status, 0);
    if (waited_pid == -1) {
#     ifndef NDEBUG
      fprintf(stderr, "waitpid returned -1\n");
#     endif
      return;
    }

    // check for pid in list of child pids
    for (int i = 0; i < n_cmds; ++i) {
      if ((*pids)[i] == waited_pid) {
#       ifndef NDEBUG
        fprintf(stderr, "process exit: %d in command list, %d left\n", waited_pid, cmds_left - 1);
#       endif

        if (--cmds_left == 0)
          return;
        break;
      }
#     ifndef NDEBUG
      else if (i == n_cmds - 1) {
        fprintf(stderr, "process exit: %d not in command list", waited_pid);
      }
#     endif
    }
  }
}

pid_t run_proc(char **argv) {
  pid_t pid = fork();
  if (pid != 0)
    return pid;

  execvp(*argv, argv);
  fprintf(stderr, "failed to execute `%s'\n", *argv);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "please supply at least one command to run\n");
    return 1;
  }

  pids_t cmds;
  int n_cmds = 0;

  {
    char *tmp_str;
    char **cmd_begin = argv + 1;
    char **cmd_end = argv + argc;

    for (char **arg_it = cmd_begin; arg_it < cmd_end; ++arg_it) {
      if (! strcmp(*arg_it, SEP)) {
        tmp_str = *arg_it;
        *arg_it = 0;
        cmds[n_cmds++] = run_proc(cmd_begin);
        *arg_it = tmp_str;
        cmd_begin = arg_it + 1;
      }
    }

    if (cmd_begin != cmd_end)
      cmds[n_cmds++] = run_proc(cmd_begin);
  }

  wait_for_exit(n_cmds, &cmds);
# ifndef NDEBUG
  fprintf(stderr, "all processes exited\n");
# endif

  return 0;
}
