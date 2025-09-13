# CIS 5480 Penn Shredder

## Student Information
- Name: Edward Sekula
- PennKey: 84768505

## Submitted Source Files
### penn-shredder/
- `penn-shredder.c`
- `penn-shredder.h`
- `split.c`
- `split.h`
- `Vec.c`
- `Vec.h`
- `panic.c`
- `panic.h`
- `vector.h`
- `Makefile`
- `.clang-tidy`

### penn-vector/ (supporting / reference implementation tests)
- `main.c`
- `Vec.c`
- `Vec.h`
- `panic.c`
- `panic.h`
- `vector.h`
- `catch.hpp`
- `catch.cpp`
- `test_basic.cpp`
- `test_macro.cpp`
- `test_panic.cpp`
- `test_suite.cpp`
- `Makefile`
- `.clang-tidy`

(If some files are duplicated between directories, clarify which is authoritative for grading.)

## Overview of Work Accomplished
This project implements a miniature shell named **penn-shredder** which:
- Displays an interactive prompt and reads user commands
- Parses input into tokens using a custom dynamic vector implementation
- Forks and executes external programs via `fork()` + `execve()`
- Supports an optional timeout argument that kills a long-running child via `alarm()` / `SIGALRM`
- Handles `SIGINT` (Ctrl+C) to terminate the foreground child process without exiting the shell
- Cleans up dynamic memory between command cycles

A supporting vector implementation (`Vec`) provides dynamic array storage for tokens and underpins command parsing.

## Code Layout / Design Decisions
### Core Components
- `penn-shredder.c`: Main REPL loop, signal setup, forking, exec, wait, and timeout logic.
- `split.c` / `split.h`: Responsible for splitting a raw input line on whitespace into tokens, returning a `Vec` of `char*` suitable for `execve`.
- `Vec.c` / `Vec.h`: Generic resizable vector with push, length, clear, and destroy operations. Serves as a safer abstraction over manual realloc logic.
- `panic.c` / `panic.h`: Centralized panic / abort helper used by vector on irrecoverable errors.

### Signals
- `SIGINT` handler (`handle_sigint`) destroys the current token vector, sends `SIGINT` to the child if running, and re-displays the prompt cleanly.
- `SIGALRM` handler (`handle_timeout`) sends `SIGKILL` to the child if execution exceeds the timeout. The presence of a timeout is determined by an optional CLI argument.

Handlers are installed each loop iteration (could be hoisted to initialization, but reinstallation is harmless and keeps logic localized).

### Memory Management
- Each command line read is tokenized; the `Vec` is either cleared (`vec_clear`) after successful wait or destroyed if empty / on interrupt.
- Care was taken to ensure no stale pointers remain after `execve()` failure paths.

### Error Handling
- `execve` failures produce a `perror("execve")` and exit the child with `EXIT_FAILURE`.
- `fork` failures abort the shell with an error message.
- Invalid timeout CLI argument causes immediate `EXIT_FAILURE`.

### Design Trade-offs
- Simplicity over feature richness (no builtin commands besides implicit `exit` on EOF).
- Reinstalled signal handlers inside the loop to avoid any potential legacy systems where handlers reset after delivery.

## How to Build
From the `penn-shredder` directory:
```
make
```
Produces the `penn-shredder` executable.

Optionally run with a timeout (seconds):
```
./penn-shredder 5
```
Then execute a long-running command, e.g.:
```
/bin/sleep 10
```
The child should be killed and the catchphrase printed.

## How to Use
Interactive session example:
```
penn-shredder# /bin/echo hello
hello
penn-shredder# /bin/sleep 10
<timeout occurs if configured>
```
Press Ctrl+C to cancel the current running child without exiting the shell. Press Ctrl+D (EOF on empty line) to exit the shell.

## Testing Strategy
- Manual tests invoking simple commands (`/bin/echo`, `/bin/ls`).
- Timeout test with `/bin/sleep` longer than provided timeout.
- SIGINT test: start `/bin/sleep 10`, press Ctrl+C, ensure shell prompt resumes.
- Edge: Empty line, Ctrl+D immediately after prompt exits cleanly.

(If automated tests are provided separately, note how they integrate.)

## Known Limitations
- Requires absolute path to executables.
- No sanitation of extremely long inputs beyond buffer size truncation.
- `SIGALRM` kill is forceful (`SIGKILL`); could attempt graceful shutdown first.

## General Comments
If any differences exist between the vector implementation expected by the autograder and this submission, they are localized to `Vec.c` / `Vec.h`. Duplicated files in both directories exist only for development convenience. Please grade the `penn-shredder` directory as authoritative for the shell implementation.