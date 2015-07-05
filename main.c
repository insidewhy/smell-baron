#define _XOPEN_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MAX_CMDS 16
#define SEP      "---"

// used as boolean by signal handlers to tell process it should exit
static int running = 1;

static void wait_for_exit(int n_cmds, pid_t *pids) {
  int cmds_left = n_cmds;

  for (;;) {
    int status;
    pid_t waited_pid = waitpid(-1, &status, 0);
    if (waited_pid == -1) {
      if (errno == EINTR) {
#       ifndef NDEBUG
        fprintf(stderr, "waitpid interrupted by signal\n");
#       endif

        if (! running)
          return;
      }
      else {
#       ifndef NDEBUG
        fprintf(stderr, "waitpid returned unhandled error state\n");
#       endif
      }
    }

    // check for pid in list of child pids
    for (int i = 0; i < n_cmds; ++i) {
      if (pids[i] == waited_pid) {
#       ifndef NDEBUG
        fprintf(stderr, "process exit: %d in command list, %d left\n", waited_pid, cmds_left - 1);
#       endif

        if (--cmds_left == 0) {
#         ifndef NDEBUG
          fprintf(stderr, "all processes exited\n");
#         endif
          return;
        }
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

static pid_t run_proc(char **argv) {
  pid_t pid = fork();
  if (pid != 0)
    return pid;

  execvp(*argv, argv);
  fprintf(stderr, "failed to execute `%s'\n", *argv);
  exit(1);
}

static void on_signal(int signum) {
# ifndef NDEBUG
  // probably not safe... at least it's only in debug mode ;)
  fprintf(stderr, "got signal %d\n", signum);
#endif
  running = 0;
}

static void perror_die(char *msg) {
  perror(msg);
  exit(1);
}

static void install_signal_handlers() {
  struct sigaction sa;
  sa.sa_handler = on_signal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL))
    perror_die("could not catch SIGINT");

  if (sigaction(SIGTERM, &sa, NULL))
    perror_die("could not catch SIGTERM");
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "please supply at least one command to run\n");
    return 1;
  }

  install_signal_handlers();

  pid_t cmds[MAX_CMDS];
  int n_cmds = 0;

  {
    char **cmd_begin = argv + 1;
    char **cmd_end = argv + argc;

    for (char **arg_it = cmd_begin; arg_it < cmd_end; ++arg_it) {
      if (! strcmp(*arg_it, SEP)) {
        *arg_it = 0; // replace with null to terminate when passed to execvp
        cmds[n_cmds++] = run_proc(cmd_begin);
        cmd_begin = arg_it + 1;
      }
    }

    if (cmd_begin < cmd_end)
      cmds[n_cmds++] = run_proc(cmd_begin);
  }

  wait_for_exit(n_cmds, cmds);

  return 0;
}
