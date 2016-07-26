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

typedef struct _CmdList {
  /* args that form command, sent to execvp */
  char **args;

  /* 1 to watch, 0 to just run the command: see -f */
  int watch;

  /* next command or null */
  struct _CmdList *next;
} CmdList;

static int run_cmds(CmdList *cmds, pid_t *watch_pids) {
  int n_cmds = 0;
  for (; cmds; cmds = cmds->next) {
    if (cmds->watch)
      watch_pids[n_cmds++] = run_proc(cmds->args);
    else
      run_proc(cmds->args);
  }
  return n_cmds;
}


#define MEMCPY_LIT(var, type, ...) memcpy(var, &(type) __VA_ARGS__, sizeof(type))

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "please supply at least one command to run\n");
    return 1;
  }

  install_term_and_int_handlers();

  pid_t watch_pids[MAX_CMDS];
  int signal_everything = 0;
  CmdList cmds = { .watch = 0, .next = NULL };
  CmdList *cmd = &cmds;

  {
    char **cmd_end = argv + argc;
    char **arg_it = argv + 1;

    int wait_on_command = 1;
    int wait_on_all_commands = 1;

    for (; arg_it < cmd_end; ++arg_it) {
      if (! strcmp(*arg_it, "-a"))
        signal_everything = 1;
      else if (! strcmp(*arg_it, "-f"))
        wait_on_all_commands = 0;
      else
        break;
    }

    cmd->args = arg_it;
    cmd->watch = wait_on_command;

    for (; arg_it < cmd_end; ++arg_it) {
      if (! strcmp(*arg_it, SEP)) {
        *arg_it = 0; // replace with null to terminate when passed to execvp

        if (arg_it + 1 == cmd_end) {
          fprintf(stderr, "command must follow `---'\n");
          exit(1);
        }

        wait_on_command = wait_on_all_commands;

        cmd->next = malloc(sizeof(CmdList));
        cmd = cmd->next;
        MEMCPY_LIT(cmd, CmdList, { .args = arg_it + 1, .watch =  wait_on_command, .next = NULL });
      }
    }
  }

  int n_cmds = run_cmds(&cmds, watch_pids);
  int error_code = wait_for_requested_commands_to_exit(n_cmds, watch_pids);
  remove_term_and_int_handlers();
  alarm(WAIT_FOR_PROC_DEATH_TIMEOUT);
  kill(signal_everything ? -1 : 0, SIGTERM);
  wait_for_all_processes_to_exit(error_code);

# ifndef NDEBUG
  fprintf(stderr, "all processes exited cleanly\n");
# endif
  return error_code;
}
