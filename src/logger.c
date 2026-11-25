#define _POSIX_C_SOURCE 200809L

#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *g_log_file = NULL;

static void get_timestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm_info;

    if (localtime_r(&now, &tm_info) != NULL) {
        strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info);
    } else {
        snprintf(buf, size, "0000-00-00 00:00:00");
    }
}

int logger_init(const char *log_path) {
    if (log_path == NULL) {
        return -1;
    }

    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    g_log_file = fopen(log_path, "a");
    if (g_log_file == NULL) {
        perror("fopen");
        return -1;
    }

    return 0;
}

void logger_log_event(const char *event_type, const char *message) {
    char timestamp[32];

    if (g_log_file == NULL) {
        return;
    }

    get_timestamp(timestamp, sizeof(timestamp));
    fprintf(g_log_file, "[%s] [%s] %s\n",
            timestamp,
            (event_type != NULL) ? event_type : "INFO",
            (message != NULL) ? message : "");
    fflush(g_log_file);
}

void logger_log_process_status(int index, pid_t pid, const char *state, int restart_count) {
    char timestamp[32];

    if (g_log_file == NULL) {
        return;
    }

    get_timestamp(timestamp, sizeof(timestamp));
    fprintf(g_log_file, "[%s] [STATUS] Worker #%d | PID: %d | State: %s | Restarts: %d\n",
            timestamp,
            index,
            (int)pid,
            (state != NULL) ? state : "UNKNOWN",
            restart_count);
    fflush(g_log_file);
}

void logger_report_status(void) {
    if (g_log_file == NULL) {
        return;
    }

    logger_log_event("STATUS_REPORT", "=== SIGUSR1 Durum Raporu Baslangici ===");

    /*
     * TODO: Öğrenci 2 (process_manager) eklendiğinde,
     * çalışan tüm alt süreçler için logger_log_process_status(...) burada çağrılacak.
     * Örnek: pm_report_all_status();
     */

    logger_log_event("STATUS_REPORT", "=== SIGUSR1 Durum Raporu Sonu ===");
}

void logger_close(void) {
    if (g_log_file != NULL) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
    }
}