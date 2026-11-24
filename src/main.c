#define _POSIX_C_SOURCE 200809L

#include "logger.h"
#include "process_manager.h"
#include "signal_manager.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_LOG_PATH "watchdog.log"

int main(int argc, char *argv[]) {
    const char *log_path = (argc > 1) ? argv[1] : DEFAULT_LOG_PATH;
    char msg_buf[256];

    
    if (logger_init(log_path) != 0) {
        fprintf(stderr, "Hata: Log dosyasi acilamadi (%s)\n", log_path);
        return EXIT_FAILURE;
    }

    snprintf(msg_buf, sizeof(msg_buf), "Watchdog baslatildi (PID: %d)", (int)getpid());
    logger_log_event("START", msg_buf);
    printf("Watchdog running (PID: %d) | Log: %s\n", (int)getpid(), log_path);
    fflush(stdout);

    
    if (signal_manager_init() != 0) {
        logger_log_event("ERROR", "Sinyal yakalayicilar kurulamadi");
        logger_close();
        return EXIT_FAILURE;
    }

  
    if (pm_init() != 0) {
        logger_log_event("ERROR", "Process manager baslatilamadi");
        logger_close();
        return EXIT_FAILURE;
    }

    if (pm_start_all() != 0) {
        logger_log_event("ERROR", "Alt surecler baslatilamadi");
        pm_stop_restarts();
        pm_send_sigterm_all();
        logger_close();
        return EXIT_FAILURE;
    }

    
    sigset_t block_mask;
    sigset_t old_mask;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGTERM);
    sigaddset(&block_mask, SIGUSR1);
    sigaddset(&block_mask, SIGCHLD);
    sigaddset(&block_mask, SIGALRM);

    while (!g_shutdown_requested) {
        
        if (g_child_exited) {
            g_child_exited = 0;
            pm_handle_sigchld();
        }

        if (g_status_requested) {
            g_status_requested = 0;
            logger_report_status();
        }

        if (g_alarm_triggered) {
            g_alarm_triggered = 0;
            pm_periodic_check();
        }

        
        if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
            perror("sigprocmask(SIG_BLOCK)");
            break;
        }

        if (!g_shutdown_requested && !g_child_exited &&
            !g_status_requested && !g_alarm_triggered) {
            sigsuspend(&old_mask);
        }

        if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
            perror("sigprocmask(SIG_SETMASK)");
            break;
        }
    }

    
    logger_close();
    return EXIT_SUCCESS;
}
