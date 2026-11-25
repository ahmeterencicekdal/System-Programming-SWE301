# Watchdog - POSIX Signal-Based Process Monitoring and Management System

Sakarya University - Software Engineering Department  
2026 - 2027 System Programming Term Project 1

## Project Overview
`watchdog` is a C-based process supervision system developed for Linux using POSIX system calls. It creates and monitors at least two child processes, detects terminations via `SIGCHLD` and `waitpid()`, automatically restarts unexpectedly terminated processes up to a maximum of 3 times, logs events with timestamps, reports process status on `SIGUSR1`, and performs a graceful shutdown on `SIGINT`/`SIGTERM`.

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