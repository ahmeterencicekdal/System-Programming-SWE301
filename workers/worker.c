#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_terminate = 0;

static void handle_sigterm(int signum) {
    (void)signum;
    g_terminate = 1;
}

static void sleep_seconds(int seconds) {
    unsigned int remaining = (seconds > 0) ? (unsigned int)seconds : 0;
    while (remaining > 0 && !g_terminate) {
        remaining = sleep(remaining);
    }
}

int main(int argc, char *argv[]) {
    const char *mode = (argc > 1) ? argv[1] : "normal";
    int duration = (argc > 2) ? atoi(argv[2]) : 5;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    if (strcmp(mode, "ignore-term") == 0) {
        /* Timeout + SIGKILL test senaryosu için SIGTERM'i yoksay */
        sa.sa_handler = SIG_IGN;
    } else {
        /* Normal graceful shutdown için SIGTERM'i yakala */
        sa.sa_handler = handle_sigterm;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("worker: sigaction(SIGTERM)");
        return EXIT_FAILURE;
    }

    /* Watchdog'un Ctrl+C (SIGINT) sinyalini doğrudan worker'ın değil watchdog'un yönetmesi için */
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("worker: sigaction(SIGINT)");
        return EXIT_FAILURE;
    }

    if (strcmp(mode, "exit-normal") == 0) {
        /* Belirli süre çalışıp normal şekilde (exit 0) sonlanır */
        sleep_seconds(duration);
        return EXIT_SUCCESS;
    }

    if (strcmp(mode, "crash") == 0) {
        /* Belirli süre çalışıp hata koduyla (exit 1) beklenmedik şekilde sonlanır */
        sleep_seconds(duration);
        return EXIT_FAILURE;
    }

    /* "normal" veya "ignore-term" modu: SIGTERM gelene kadar çalışmaya devam eder */
    while (!g_terminate) {
        sleep(1);
    }

    return EXIT_SUCCESS;
}