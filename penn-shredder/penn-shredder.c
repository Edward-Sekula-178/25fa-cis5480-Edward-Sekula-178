#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <penn-shredder.h>
#include <split.h>
#include "../penn-vector/Vec.h"

static volatile sig_atomic_t got_sigint = 0;
static volatile sig_atomic_t timed_out = 0;
static volatile sig_atomic_t child_alive = 0;
static volatile sig_atomic_t child_killed = 0;
static pid_t child_pid = -1;

static void on_sigint(int sig) {
  (void)sig;
  got_sigint = 1;
  if (child_alive && child_pid > 0) {
    kill(child_pid, SIGINT);
  } else {
    /* Print newline to move to a fresh prompt */
    const char nl = '\n';
    write(STDERR_FILENO, &nl, 1);
  }
}

static void on_sigalrm(int sig) {
  (void)sig;
  timed_out = 1;
  if (child_alive && child_pid > 0) {
    kill(child_pid, SIGKILL);
    child_killed = 1;
  }
}

static ssize_t read_line(char* buf, size_t cap) {
  for (;;) {
    ssize_t r = read(STDIN_FILENO, buf, cap);
    if (r < 0 && errno == EINTR)
      continue;
    return r;
  }
}

int main(int argc, char* argv[]) {
  int timeout = 0;
  if (argc > 2)
    return EXIT_FAILURE;
  if (argc == 2) {
    char* end = NULL;
    long v = strtol(argv[1], &end, 10);
    if (*end != '\0' || v < 0 || v > INT_MAX)
      return EXIT_FAILURE;
    timeout = (int)v;
  }

  struct sigaction sa = {0};
  sa.sa_handler = on_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction(SIGINT)");
    return EXIT_FAILURE;
  }

  struct sigaction sa_alrm = {0};
  sa_alrm.sa_handler = on_sigalrm;
  sigemptyset(&sa_alrm.sa_mask);
  sa_alrm.sa_flags = 0;
  if (sigaction(SIGALRM, &sa_alrm, NULL) == -1) {
    perror("sigaction(SIGALRM)");
    return EXIT_FAILURE;
  }

  for (;;) {
    got_sigint = 0;
    timed_out = 0;
    child_killed = 0;
    child_alive = 0;
    child_pid = -1;

    /* Prompt to stderr per spec */
    write(STDERR_FILENO, PROMPT, strlen(PROMPT));

    /* Read input */
    const size_t max_line_len = 4096;
    char cmd[max_line_len];
    ssize_t nr = read_line(cmd, sizeof(cmd));
    if (nr == 0) {
      /* EOF at beginning of line: exit */
      const char nl = '\n';
      write(STDERR_FILENO, &nl, 1);
      _exit(EXIT_SUCCESS);
    }
    if (nr < 0) {
      perror("read");
      return EXIT_FAILURE;
    }

    size_t n = (size_t)nr;
    if (n == sizeof(cmd))
      n = sizeof(cmd) - 1; /* ensure space for NUL */
    cmd[n] = '\0';

    /* Tokenize (split mutates cmd) */
    Vec tokens = split_string(cmd, " \t\n");
    size_t argc_exec = vec_len(&tokens);
    if (argc_exec == 0) {
      vec_destroy(&tokens);
      continue; /* empty or all whitespace: ignore */
    }

    /* Build null-terminated argv with malloc as required */
    char** argv_exec = malloc((argc_exec + 1) * sizeof(char*));
    if (!argv_exec) {
      perror("malloc");
      vec_destroy(&tokens);
      return EXIT_FAILURE;
    }
    for (size_t i = 0; i < argc_exec; ++i) {
      argv_exec[i] = (char*)vec_at(&tokens, i);
    }
    argv_exec[argc_exec] = NULL;

    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      free(argv_exec);
      vec_destroy(&tokens);
      return EXIT_FAILURE;
    }

    if (pid == 0) {
      /* Child: cancel any inherited alarm just in case */
      alarm(0);
      extern char** environ;
      execve(argv_exec[0], argv_exec, environ);
      perror("execve");
      _exit(EXIT_FAILURE);
    }

    /* Parent */
    child_pid = pid;
    child_alive = 1;

    if (timeout > 0)
      alarm(timeout);

    int status = 0;
    for (;;) {
      pid_t w = waitpid(pid, &status, 0);
      if (w == -1 && errno == EINTR)
        continue;
      if (w == -1) {
        perror("waitpid");
        break;
      }
      break;
    }

    /* Child is no longer running */
    child_alive = 0;
    alarm(0); /* cancel pending alarm if any */

    /* Print catchphrase if timed out (either parent killed or child died to
     * SIGALRM) */
    int say_catchphrase = 0;
    if (timed_out || (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)) {
      say_catchphrase = 1;
    }
    if (say_catchphrase) {
      write(STDOUT_FILENO, CATCHPHRASE, strlen(CATCHPHRASE));
    }

    free(argv_exec);
    vec_destroy(&tokens);
  }
}
