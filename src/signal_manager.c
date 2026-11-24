#define _POSIX_C_SOURCE 200809L

#include "signal_manager.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

volatile sig_atomic_t g_shutdown_requested = 0;
volatile sig_atomic_t g_shutdown_signal = 0;
volatile sig_atomic_t g_status_requested = 0;
volatile sig_atomic_t g_child_exited = 0;
volatile sig_atomic_t g_alarm_triggered = 0;


static void handle_shutdown_signal(int signum) {
    int saved_errno = errno;

    if (!g_shutdown_requested) {
        g_shutdown_signal = signum;
        g_shutdown_requested = 1;
    }

    errno = saved_errno;
}


static void handle_sigusr1(int signum) {
    int saved_errno = errno;
    (void)signum;

    g_status_requested = 1;

    errno = saved_errno;
}


static void handle_sigchld(int signum) {
    int saved_errno = errno;
    (void)signum;

    g_child_exited = 1;

    errno = saved_errno;
}


static void handle_sigalrm(int signum) {
    int saved_errno = errno;
    (void)signum;

    g_alarm_triggered = 1;

    errno = saved_errno;
}



static int register_signal(int signum, void (*handler)(int), int flags) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;

    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    return 0;
}



int signal_manager_init(void) {
    if (register_signal(SIGINT, handle_shutdown_signal, 0) == -1) {
        return -1;
    }
    if (register_signal(SIGTERM, handle_shutdown_signal, 0) == -1) {
        return -1;
    }
    if (register_signal(SIGUSR1, handle_sigusr1, 0) == -1) {
        return -1;
    }
    if (register_signal(SIGALRM, handle_sigalrm, 0) == -1) {
        return -1;
    }

    if (register_signal(SIGCHLD, handle_sigchld, SA_NOCLDSTOP) == -1) {
        return -1;
    }

    if (register_signal(SIGPIPE, SIG_IGN, 0) == -1) {
        return -1;
    }

    return 0;
}


const char *signal_manager_get_name(int signum) {
    switch (signum) {
        case SIGINT:  return "SIGINT";
        case SIGTERM: return "SIGTERM";
        case SIGUSR1: return "SIGUSR1";
        case SIGCHLD: return "SIGCHLD";
        case SIGALRM: return "SIGALRM";
        default:      return "UNKNOWN";
    }
}
