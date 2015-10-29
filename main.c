#define _XOPEN_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <limits.h>

#define WAIT_FOR_PROC_DEATH_TIMEOUT 10
#define MAX_CMDS 16
#define SEP      "---"


// TODO: sig_atomic_t (although it is typedefd to int :P)
// used as boolean by signal handlers to tell process it should exit
static volatile int running = 1;

static int wait_for_requested_commands_to_exit(int n_cmds, pid_t *pids) {
  int cmds_left = n_cmds;
  int error_code = 0;
  int error_code_idx = INT_MAX;

  for (;;) {
    int status;

    if (! running) return error_code;
    pid_t waited_pid = waitpid(-1, &status, 0);
    if (waited_pid == -1) {
      if (errno == EINTR) {
#       ifndef NDEBUG
        fprintf(stderr, "waitpid interrupted by signal\n");
#       endif
      }
      else {
#       ifndef NDEBUG
        fprintf(stderr, "waitpid returned unhandled error state\n");
#       endif
      }
    }
    if (! running) return error_code;

    // check for pid in list of child pids
    for (int i = 0; i < n_cmds; ++i) {

      if (pids[i] == waited_pid) {
        if (WIFEXITED(status)) {
          int exit_status = WEXITSTATUS(status);
#         ifndef NDEBUG
          fprintf(stderr, "process exit with status: %d \n", exit_status);
#         endif
          if (exit_status != 0 && i < error_code_idx) {
            error_code = exit_status;
            error_code_idx = i;
          }
        }

#       ifndef NDEBUG
        fprintf(stderr, "process exit: %d in command list, %d left\n", waited_pid, cmds_left - 1);
#       endif

        if (--cmds_left == 0) {
#         ifndef NDEBUG
          fprintf(stderr, "all processes exited\n");
#         endif
          return error_code;
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

  return error_code;
}

static void wait_for_all_processes_to_exit() {
  int status;
  for (;;) {
    if (waitpid(-1, &status, 0) == -1 && errno == ECHILD)
      return;
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

static void on_alarm(int signum) {
# ifndef NDEBUG
  // probably not safe... at least it's only in debug mode ;)
  fprintf(stderr, "timeout waiting for child processes to die\n");
#endif
  exit(0);
}

static void perror_die(char *msg) {
  perror(msg);
  exit(1);
}

static void install_term_and_int_handlers() {
  struct sigaction sa;
  sa.sa_handler = on_signal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL))
    perror_die("could not catch SIGINT");

  if (sigaction(SIGTERM, &sa, NULL))
    perror_die("could not catch SIGTERM");

  sa.sa_handler = on_alarm;
  if (sigaction(SIGALRM, &sa, NULL))
    perror_die("could not catch SIGALRM");
}

static void remove_term_and_int_handlers() {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, NULL))
    return;
  if (sigaction(SIGTERM, &sa, NULL))
    return;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "please supply at least one command to run\n");
    return 1;
  }

  install_term_and_int_handlers();

  pid_t cmds[MAX_CMDS];
  int n_cmds = 0;
  {
    char **cmd_end = argv + argc;
    char **arg_it = argv + 1;

    int wait_on_command = 1;
    int wait_on_all_commands = 1;

    // TODO: parse more commands, including -h/--help with getopt
    if (! strcmp(*arg_it, "-f")) {
      ++arg_it;
      wait_on_all_commands = 0;
    }

    char **cmd_begin = arg_it;

    for (; arg_it < cmd_end; ++arg_it) {
      if (! strcmp(*arg_it, SEP)) {
        *arg_it = 0; // replace with null to terminate when passed to execvp
        if (wait_on_command)
          cmds[n_cmds++] = run_proc(cmd_begin);
        else
          run_proc(cmd_begin);

        cmd_begin = arg_it + 1;
        wait_on_command = wait_on_all_commands;
      }
    }

    if (cmd_begin < cmd_end) {
      if (wait_on_command)
        cmds[n_cmds++] = run_proc(cmd_begin);
      else
        run_proc(cmd_begin);

      wait_on_command = wait_on_all_commands;
    }
  }

  int error_code = wait_for_requested_commands_to_exit(n_cmds, cmds);
  remove_term_and_int_handlers();
  alarm(WAIT_FOR_PROC_DEATH_TIMEOUT);
  kill(0, SIGTERM);
  wait_for_all_processes_to_exit();

# ifndef NDEBUG
  fprintf(stderr, "all processes exited cleanly\n");
# endif
  return error_code;
}
