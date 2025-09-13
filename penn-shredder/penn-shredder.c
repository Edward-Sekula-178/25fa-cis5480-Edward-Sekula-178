/**
 * @file penn-shredder.c
 * @brief Minimal shell that executes a command with an optional timeout.
 *
 * Spec highlights:
 *  - Prompt to STDERR. Ctrl-D at empty line exits. Ctrl-C forwards to child or
 * re-prompts.
 *  - Only: alarm, execve, exit/_exit, fork, kill, read, sigaction, wait, write
 *    + strtol, exit, free, malloc, perror, strlen
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>  /* perror */
#include <stdlib.h> /* malloc, free, strtol, exit */
#include <string.h> /* strlen */
#include <sys/wait.h>
#include <unistd.h> /* write, read, fork, execve, alarm, _exit, kill */

#include "../penn-vector/Vec.h"
#include "penn-shredder.h"
#include "split.h"

/* -------------------- constants -------------------- */

enum { MAX_LINE_LEN = 4096 };

/* -------------------- signal-visible state -------------------- */

static volatile sig_atomic_t timed_out = 0;
static volatile sig_atomic_t child_alive = 0;
static pid_t child_pid = -1;

/* -------------------- handlers -------------------- */

static void on_sigint(int sig) {
  (void)sig;
  if (child_alive && child_pid > 0) {
    kill(child_pid, SIGINT);
    (void)write(STDERR_FILENO, "\n", 1);
  } else {
    (void)write(STDERR_FILENO, "\n", 1);
  }
}

static void on_sigalrm(int sig) {
  (void)sig;
  timed_out = 1;
  if (child_alive && child_pid > 0) {
    kill(child_pid, SIGKILL);
  }
}

/* -------------------- helpers (non-signal context) -------------------- */

static int install_handler(int signo, void (*handler)(int)) {
  struct sigaction sa_h = (struct sigaction){0};
  sa_h.sa_handler = handler;
  sigemptyset(&sa_h.sa_mask);
  sa_h.sa_flags = 0;
  if (sigaction(signo, &sa_h, NULL) == -1) {
    perror("sigaction");
    return -1;
  }
  return 0;
}

static void write_prompt(void) {
  (void)write(STDERR_FILENO, PROMPT, strlen(PROMPT));
}

static int parse_timeout(int argc, char** argv, int* timeout_out) {
  if (argc > 2) {
    return -1;
  }
  *timeout_out = 0;
  if (argc == 2) {
    char* end = NULL;
    *timeout_out = (int)strtol(argv[1], &end, 10);
    if (*timeout_out < 0) {
      return -1;
    }
  }
  return 0;
}

typedef enum { RR_OK, RR_EOF, RR_INTR, RR_ERR } ReadRes;

static ReadRes read_command(char* buf, size_t cap, ssize_t* n_out) {
  ssize_t n_read = read(STDIN_FILENO, buf, cap);
  if (n_read == 0) {
    return RR_EOF;
  }
  if (n_read < 0) {
    if (errno == EINTR) {
      return RR_INTR;
    }
    perror("read");
    return RR_ERR;
  }
  if ((size_t)n_read == cap) {
    n_read = (ssize_t)cap - 1;
  }
  buf[n_read] = '\0';
  *n_out = n_read;
  return RR_OK;
}

static ssize_t build_argv(Vec* tokens, char*** out_argv) {
  size_t argc_exec = vec_len(tokens);
  if (argc_exec == 0) {
    *out_argv = NULL;
    return 0;
  }
  char** argv_exec = (char**)malloc((argc_exec + 1) * sizeof(char*));
  if (!argv_exec) {
    perror("malloc");
    *out_argv = NULL;
    return -1;
  }
  for (size_t i = 0; i < argc_exec; ++i) {
    argv_exec[i] = (char*)vec_get(tokens, i); /* token points into cmd buffer */
  }
  argv_exec[argc_exec] = NULL;
  *out_argv = argv_exec;
  return (ssize_t)argc_exec;
}

static int reap_one_child(int* status_out) {
  int status = 0;
  for (;;) {
    pid_t w_pid = wait(&status);
    if (w_pid == -1 && errno == EINTR) {
      continue;
    }
    if (w_pid == -1) {
      perror("wait");
      return -1;
    }
    break;
  }
  if (status_out) {
    *status_out = status;
  }
  return 0;
}

/* Spawn, enforce timeout, print catchphrase if needed. Returns 0/EXIT_FAILURE.
 */
static int run_command(char** argv_exec, int timeout_secs) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    alarm(0); /* child must not time out itself */
    extern char** environ;
    execve(argv_exec[0], argv_exec, environ);
    perror("execve");
    _exit(EXIT_FAILURE);
  }

  /* parent */
  child_pid = pid;
  child_alive = 1;
  timed_out = 0;
  if (timeout_secs > 0) {
    alarm((unsigned)timeout_secs);
  }

  int status = 0;
  (void)reap_one_child(&status);

  child_alive = 0;
  alarm(0);

  if (timed_out || (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)) {
    (void)write(STDOUT_FILENO, CATCHPHRASE, strlen(CATCHPHRASE));
  }
  return 0;
}

/* -------------------- main -------------------- */

int main(int argc, char* argv[]) {
  int timeout = 0;
  if (parse_timeout(argc, argv, &timeout) != 0) {
    return EXIT_FAILURE;
  }
  if (install_handler(SIGINT, on_sigint) == -1) {
    return EXIT_FAILURE;
  }
  if (install_handler(SIGALRM, on_sigalrm) == -1) {
    return EXIT_FAILURE;
  }

  for (;;) {
    write_prompt();

    char cmd[MAX_LINE_LEN];
    ssize_t n_read = 0;
    switch (read_command(cmd, sizeof(cmd), &n_read)) {
      case RR_EOF:
        (void)write(STDERR_FILENO, "\n", 1);
        _exit(EXIT_SUCCESS);
      case RR_INTR:
        /* interrupted while idle: just re-prompt */
        continue;
      case RR_ERR:
        return EXIT_FAILURE;
      case RR_OK:
        break;
    }

    /* Tokenize and build argv */
    Vec tokens = split_string(cmd, " \t\n");
    char** argv_exec = NULL;
    ssize_t argc_exec = build_argv(&tokens, &argv_exec);
    if (argc_exec <= 0) {
      vec_destroy(&tokens);
      if (argc_exec < 0) {
        return EXIT_FAILURE; /* malloc failure */
      }
      continue; /* empty line */
    }

    /* Run */
    int rc_out = run_command(argv_exec, timeout);

    /* Cleanup */
    free(argv_exec);
    vec_destroy(&tokens);

    if (rc_out != 0) {
      return EXIT_FAILURE;
    }
  }
}