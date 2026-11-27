# Watchdog - POSIX Signal-Based Process Monitoring and Management System

2026 - 2027 System Programming (SWE301) Project 1

## Project Overview
`watchdog` is a C-based process supervision system built for Linux using POSIX system calls. It spawns and monitors 4 child worker processes, tracks terminations via `SIGCHLD` and non-blocking `waitpid()`, automatically restarts unexpectedly terminated processes up to a maximum of 3 attempts, performs periodic health checks via `SIGALRM` (`setitimer`), logs all lifecycle events with timestamps, generates on-demand status reports on `SIGUSR1`, and executes a graceful shutdown with a 5-second timeout (`SIGTERM` followed by `SIGKILL` fallback) on `SIGINT`/`SIGTERM`.

## Team Members & Responsibilities

| Role | Student Name | Student ID | Primary Modules |
| :--- | :--- | :--- | :--- |
| **Student 1** | Ömer Memiş | B221202069 | `signal_manager.h/c`, `main.c` (Signal handling, `sigsuspend` loop, graceful shutdown) |
| **Student 2** | Muhammet Karaman | B221202052 | `process_manager.h/c` (`fork`/`execv`, `waitpid` reaping, 3-restart limit, `SIGALRM` check) |
| **Student 3** | Ahmet Eren Çiçekdal | B231202020 | `logger.h/c`, `workers/worker.c`, `tests/test_scenarios.sh`, `Makefile`, `README.md`, `REPORT.pdf` |

## Project Structure
```text
watchdog-project/
├── Makefile
├── README.md
├── include/
│   ├── signal_manager.h
│   ├── process_manager.h
│   └── logger.h
├── src/
│   ├── main.c
│   ├── signal_manager.c
│   ├── process_manager.c
│   └── logger.c
├── workers/
│   └── worker.c
├── tests/
│   └── test_scenarios.sh
└── docs/
    └── REPORT.pdf
```

## Managed Worker Modes
By default, `watchdog` spawns 4 instances of `./workers/worker` to demonstrate all required behaviors:
* `worker-1` (`normal`): Runs continuously until terminated by `SIGTERM`.
* `worker-2` (`normal`): Runs continuously until terminated by `SIGTERM`.
* `worker-3` (`exit-normal`): Exits cleanly with code `0` after a fixed duration (not restarted).
* `worker-4` (`crash`): Exits with error code `1` to verify bounded automatic restarts (up to 3/3 limit).

*(Note: `./workers/worker ignore-term` is also supported to test the 5-second `SIGTERM` timeout and `SIGKILL` fallback.)*

## Build & Installation
Compile both `watchdog` and `workers/worker` with strict compiler flags (`-Wall -Wextra -Werror`):

```bash
make
```

Clean compiled binaries, object files, and generated logs:

```bash
make clean
```

## Usage
Start the watchdog (defaults to `watchdog.log`):

```bash
./watchdog
```

Or specify a custom log file path:

```bash
./watchdog custom.log
```

### Runtime Signal Commands (from another terminal)
* **On-Demand Status Report (`SIGUSR1`):** Logs PID, state (`RUNNING`, `EXITED_OK`, `FAILED`), and restart count for every child process.
  ```bash
  kill -SIGUSR1 <WATCHDOG_PID>
  ```
* **Graceful Shutdown (`SIGTERM` or `Ctrl+C`):** Disables restarts and periodic timer, sends `SIGTERM` to active children, waits up to 5 seconds, and sends `SIGKILL` to any remaining processes.
  ```bash
  kill -SIGTERM <WATCHDOG_PID>
  ```
* **Simulate Unexpected Crash (`SIGKILL`):**
  ```bash
  kill -SIGKILL <WORKER_PID>
  ```

## Running Tests
Execute the automated test suite covering build verification, normal operation, `SIGUSR1` reporting, `SIGKILL` recovery, normal exit handling, 3/3 restart limit enforcement, and `SIGTERM` graceful shutdown:

```bash
chmod +x tests/test_scenarios.sh
./tests/test_scenarios.sh
```

Interactive `Ctrl+C` (`SIGINT`) shutdown can be verified directly by running `./watchdog` in the terminal and pressing `Ctrl+C`.

## Documentation
Full architectural details, test validation results, sample runtime logs, and commit history are available in [`docs/REPORT.pdf`](docs/REPORT.pdf).