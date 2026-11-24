#define _POSIX_C_SOURCE 200809L

#include "logger.h"
#include "process_manager.h"
#include "signal_manager.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_LOG_PATH "watchdog.log"
#define SHUTDOWN_TIMEOUT_SEC   5.0
#define POLL_INTERVAL_NS       100000000L

static void perform_graceful_shutdown(void) {
    char msg_buf[256];
    int shutdown_sig = (int)g_shutdown_signal;
    struct timespec start_time;
    struct timespec now;
    struct timespec poll_interval;

    /* 1. Adım: Otomatik yeniden başlatmayı ve periyodik zamanlayıcıyı kapat */
    pm_stop_restarts();
    alarm(0);

    /* 2. Adım: Kapanış başlangıcını zaman damgasıyla logla */
    snprintf(msg_buf, sizeof(msg_buf),
             "Zarif kapanis baslatildi (Alinan sinyal: %s)",
             signal_manager_get_name(shutdown_sig));
    logger_log_event("SHUTDOWN", msg_buf);
    printf("\n%s\n", msg_buf);
    fflush(stdout);

    /* 3. Adım: Çalışan tüm alt süreçlere SIGTERM gönder */
    pm_send_sigterm_all();

    /* 4. Adım: Belirlenen timeout (5 sn) boyunca kapanan çocukları topla */
    poll_interval.tv_sec = 0;
    poll_interval.tv_nsec = POLL_INTERVAL_NS;

    if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
        perror("clock_gettime");
    }

    while (1) {
        /* Sonlanan çocukları waitpid() ile toplayıp süreç tablosunu güncelle */
        pm_handle_sigchld();

        /* Tüm alt süreçler süre dolmadan kapandıysa beklemeyi erken bitir */
        if (pm_active_count() <= 0) {
            break;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
            double elapsed = (double)(now.tv_sec - start_time.tv_sec) +
                             (double)(now.tv_nsec - start_time.tv_nsec) / 1e9;
            if (elapsed >= SHUTDOWN_TIMEOUT_SEC) {
                break;
            }
        }

        if (nanosleep(&poll_interval, NULL) == -1 && errno != EINTR) {
            perror("nanosleep");
            break;
        }
    }

    /* 5. Adım: Dirençli kalan süreçlere SIGKILL gönder ve zombi kalmamasını sağla */
    pm_handle_sigchld();
    if (pm_active_count() > 0) {
        snprintf(msg_buf, sizeof(msg_buf),
                 "Timeout (%.1f sn) doldu; kapanmayan %d alt surece SIGKILL gonderiliyor",
                 SHUTDOWN_TIMEOUT_SEC, pm_active_count());
        logger_log_event("SHUTDOWN_FORCE", msg_buf);

        pm_force_kill_remaining();

        /* SIGKILL sonrası çekirdeğin süreçleri sonlandırması için kısa bekleme ve reap */
        struct timespec kill_wait = {0, 50000000L}; /* 50 ms */
        nanosleep(&kill_wait, NULL);
        pm_handle_sigchld();
    }

    logger_log_event("SHUTDOWN_COMPLETE", "Tum alt surecler temizlendi, watchdog sonlaniyor");
    logger_close();
}

int main(int argc, char *argv[]) {
    const char *log_path = (argc > 1) ? argv[1] : DEFAULT_LOG_PATH;
    char msg_buf[256];

    /* 1. Adım: Log sistemini başlat ve watchdog PID bilgisini kaydet */
    if (logger_init(log_path) != 0) {
        fprintf(stderr, "Hata: Log dosyasi acilamadi (%s)\n", log_path);
        return EXIT_FAILURE;
    }

    snprintf(msg_buf, sizeof(msg_buf), "Watchdog baslatildi (PID: %d)", (int)getpid());
    logger_log_event("START", msg_buf);
    printf("Watchdog running (PID: %d) | Log: %s\n", (int)getpid(), log_path);
    fflush(stdout);

    /*
     * 2. Adım: Sinyal yakalayıcıları alt süreçler oluşturulmadan önce kur.
     * Böylece alt süreç anında sonlansa bile SIGCHLD sinyali kaçırılmaz.
     */
    if (signal_manager_init() != 0) {
        logger_log_event("ERROR", "Sinyal yakalayicilar kurulamadi");
        logger_close();
        return EXIT_FAILURE;
    }

    /* 3. Adım: Alt süreç yöneticisini başlat ve en az iki alt süreci ayağa kaldır */
    if (pm_init() != 0) {
        logger_log_event("ERROR", "Process manager baslatilamadi");
        logger_close();
        return EXIT_FAILURE;
    }

    if (pm_start_all() != 0) {
        logger_log_event("ERROR", "Alt surecler baslatilamadi");
        pm_stop_restarts();
        pm_send_sigterm_all();
        pm_handle_sigchld();
        logger_close();
        return EXIT_FAILURE;
    }

    /*
     * 4. Adım: Olay tabanlı ana kontrol döngüsü.
     * Sinyallerin bayrak kontrolü ile bekleme arasında kaçırılmaması için
     * sigprocmask() + sigsuspend() ikilisi kullanılır.
     */
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

        /* Bayraklar boşsa atomik olarak sinyal bekle */
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

    /* 5. Adım: Zarif kapanış sekansını çalıştır */
    perform_graceful_shutdown();
    return EXIT_SUCCESS;
}
