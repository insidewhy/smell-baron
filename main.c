#define _XOPEN_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <limits.h>

#define WAIT_FOR_PROC_DEATH_TIMEOUT 10
#define SEP      "---"

// TODO: sig_atomic_t (although it is typedefd to int :P)
// used as boolean by signal handlers to tell process it should exit
static volatile int running = 1;

typedef struct {
  /* args that form command, sent to execvp */
  char **args;

  /* 1 to watch, 0 to just run the command: see -f */
  bool watch;

  pid_t pid;
} Cmd;

typedef struct {
  bool signal_everything;
} Opts;

static int wait_for_requested_commands_to_exit(int n_watch_cmds, Cmd **watch_cmds) {
  int cmds_left = n_watch_cmds;
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
    for (int i = 0; i < n_watch_cmds; ++i) {

      if (watch_cmds[i]->pid == waited_pid) {
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
      else if (i == n_watch_cmds - 1) {
        fprintf(stderr, "process exit: %d not in watched commands list\n", waited_pid);
      }
#     endif
    }
  }

  return error_code;
}

static int alarm_exit_code = 0;
static void wait_for_all_processes_to_exit(int error_code) {
  int status;
  alarm_exit_code = error_code;
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
  exit(alarm_exit_code);
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

static void run_cmds(int n_cmds, Cmd *cmds) {
  for (int i = 0; i < n_cmds; ++i)
    cmds[i].pid = run_proc(cmds[i].args);
}

static void parse_cmd_args(Opts *opts, Cmd *cmd, char **arg_it, char **args_end) {
  int c;
  optind = 1; // reset global used as pointer by getopt
  while ((c = getopt(args_end - arg_it, arg_it - 1, "+af")) != -1) {
    switch(c) {
      case 'a':
        if (getpid() != 1) {
          fprintf(stderr, "-a can only be used from the init process (a process with pid 1)\n");
          exit(1);
        }
        opts->signal_everything = true;
        break;

      case 'f':
        cmd->watch = true;
    }
  }
  cmd->args = arg_it + optind - 1;
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "please supply at least one command to run\n");
    return 1;
  }

  install_term_and_int_handlers();

  Cmd *cmds;
  Opts opts;
  int n_watch_cmds = 0;
  int n_cmds = 1;

  {
    char **args_end = argv + argc;

    for (char **i = argv + 1; i < args_end; ++i) {
      if (! strcmp(*i, SEP))
        ++n_cmds;
    }

    cmds = calloc(n_cmds, sizeof(Cmd));
    opts = (Opts) { .signal_everything = false };
    parse_cmd_args(&opts, cmds, argv + 1, args_end);
    if (cmds->watch)
      ++n_watch_cmds;

    char **arg_it = cmds->args;

    int cmd_idx = 0;
    for (; arg_it < args_end; ++arg_it) {
      if (! strcmp(*arg_it, SEP)) {
        *arg_it = 0; // replace with null to terminate when passed to execvp

        if (arg_it + 1 == args_end) {
          fprintf(stderr, "command must follow `---'\n");
          exit(1);
        }

        parse_cmd_args(&opts, cmds + (++cmd_idx), arg_it + 1, args_end);
        Cmd *cmd = cmds + cmd_idx;
        if (cmd->watch)
          ++n_watch_cmds;
        arg_it = cmd->args - 1;
      }
    }
  }

  // if -f hasn't been used then watch every command
  if (0 == n_watch_cmds) {
    n_watch_cmds = n_cmds;
    for (int i = 0; i < n_cmds; ++i)
      cmds[i].watch = true;
  }

  Cmd **watch_cmds = calloc(n_watch_cmds, sizeof(Cmd **));
  {
    int watch_cmd_end = 0;
    for (int i = 0; i < n_cmds; ++i) {
      if (cmds[i].watch)
        watch_cmds[watch_cmd_end++] = cmds + i;
    }
  }

  run_cmds(n_cmds, cmds);
  int error_code = wait_for_requested_commands_to_exit(n_watch_cmds, watch_cmds);
  remove_term_and_int_handlers();
  alarm(WAIT_FOR_PROC_DEATH_TIMEOUT);
  kill(opts.signal_everything ? -1 : 0, SIGTERM);
  wait_for_all_processes_to_exit(error_code);

# ifndef NDEBUG
  fprintf(stderr, "all processes exited cleanly\n");
# endif
  return error_code;
}
