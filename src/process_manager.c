/*
 * process_manager.c - Alt surec yonetimi (Student 2)
 *
 *  - fork() + execv() ile alt surec baslatma
 *  - (commit 2) SIGCHLD + waitpid, normal/beklenmeyen ayrimi, restart
 *  - (commit 3) setitimer() + SIGALRM ile periyodik kontrol
 *
 * Not: Sinyal handler'lari signal_manager.c'dedir ve sadece bayrak set eder.
 * Bu dosyadaki fonksiyonlar ana donguden (main.c) cagrilir, handler'dan DEGIL.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "process_manager.h"
#include "logger.h"

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Baslatilacak alt sureclerin tanimi: { isim, worker modu, sure } */
typedef struct {
    const char *name;
    const char *mode;
    const char *duration;
} worker_spec_t;

static const worker_spec_t g_specs[] = {
    { "worker-1", "normal",      "0" }, /* SIGTERM gelene kadar calisir     */
    { "worker-2", "normal",      "0" }, /* SIGTERM gelene kadar calisir     */
    { "worker-3", "exit-normal", "8" }, /* 8 sn sonra exit(0): normal bitis */
    { "worker-4", "crash",       "4" }, /* 4 sn sonra exit(1): restart testi */
};

/* Her alt surec icin tutulan bilgi */
typedef struct {
    char          name[32];
    char          mode[16];
    char          duration[16];
    pid_t         pid;           /* calisiyorsa PID, degilse -1 */
    child_state_t state;
    int           restart_count;
    int           last_status;   /* waitpid'in son dondurdugu status */
    time_t        start_time;
} child_info_t;

static child_info_t g_children[PM_MAX_CHILDREN];
static int          g_child_count = 0;
static int          g_restarts_enabled = 1;

/* ------------------------------------------------------------------ */
/* Yardimci fonksiyonlar                                               */
/* ------------------------------------------------------------------ */

/* printf tarzinda mesaj olusturup logger'a gonderir */
static void pm_log(const char *event, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static void pm_log(const char *event, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    logger_log_event(event, buf);
}

static void copy_str(char *dst, size_t size, const char *src)
{
    size_t len = strlen(src);

    if (len >= size)
        len = size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

/* Tek bir alt sureci fork + execv ile baslatir */
static int spawn_child(child_info_t *c)
{
    pid_t pid = fork();

    if (pid == -1) {
        pm_log("ERROR", "%s icin fork() basarisiz: %s", c->name, strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* --- COCUK SUREC --- */
        char *argv[4];
        sigset_t empty;

        /* Ebeveynde bloklu olabilecek sinyalleri cocukta ac */
        sigemptyset(&empty);
        sigprocmask(SIG_SETMASK, &empty, NULL);

        argv[0] = (char *)PM_WORKER_PATH;
        argv[1] = c->mode;
        argv[2] = c->duration;
        argv[3] = NULL;

        execv(PM_WORKER_PATH, argv);

        /* execv sadece hata olursa geri doner */
        perror("execv");
        _exit(127);
    }

    /* --- EBEVEYN (watchdog) --- */
    c->pid = pid;
    c->state = CHILD_RUNNING;
    c->start_time = time(NULL);

    pm_log("CHILD_START", "%s baslatildi (PID: %d, mod: %s, restart: %d/%d)",
           c->name, (int)pid, c->mode, c->restart_count, PM_MAX_RESTARTS);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Disariya acik fonksiyonlar                                          */
/* ------------------------------------------------------------------ */

int pm_init(void)
{
    int i;
    int n = (int)(sizeof(g_specs) / sizeof(g_specs[0]));

    if (n > PM_MAX_CHILDREN) {
        pm_log("ERROR", "Cok fazla alt surec tanimli (%d > %d)", n, PM_MAX_CHILDREN);
        return -1;
    }

    memset(g_children, 0, sizeof(g_children));
    for (i = 0; i < n; i++) {
        copy_str(g_children[i].name, sizeof(g_children[i].name), g_specs[i].name);
        copy_str(g_children[i].mode, sizeof(g_children[i].mode), g_specs[i].mode);
        copy_str(g_children[i].duration, sizeof(g_children[i].duration), g_specs[i].duration);
        g_children[i].pid = -1;
        g_children[i].state = CHILD_NOT_STARTED;
    }
    g_child_count = n;
    g_restarts_enabled = 1;

    pm_log("PM_INIT", "Process manager hazir (%d alt surec, restart limiti: %d)",
           n, PM_MAX_RESTARTS);
    return 0;
}

int pm_start_all(void)
{
    int i;
    int started = 0;

    for (i = 0; i < g_child_count; i++) {
        if (g_children[i].state == CHILD_NOT_STARTED &&
            spawn_child(&g_children[i]) == 0)
            started++;
    }

    /* Proje en az iki alt surec istiyor */
    return (started >= 2) ? 0 : -1;
}

void pm_handle_sigchld(void)
{
    /* TODO (commit 2): waitpid ile toplama + normal/hatali ayrimi + restart */
}

void pm_periodic_check(void)
{
    /* TODO (commit 3): setitimer + SIGALRM ile periyodik kontrol */
}

void pm_stop_restarts(void)
{
    g_restarts_enabled = 0;
    pm_log("PM_STOP", "Otomatik restart durduruldu");
}

void pm_send_sigterm_all(void)
{
    int i;

    for (i = 0; i < g_child_count; i++) {
        child_info_t *c = &g_children[i];

        if (c->state == CHILD_RUNNING && c->pid > 0) {
            if (kill(c->pid, SIGTERM) == 0)
                pm_log("SIGTERM_SENT", "%s (PID: %d) alt surecine SIGTERM gonderildi",
                       c->name, (int)c->pid);
            else
                pm_log("ERROR", "%s (PID: %d) icin kill(SIGTERM) basarisiz: %s",
                       c->name, (int)c->pid, strerror(errno));
        }
    }
}

void pm_force_kill_remaining(void)
{
    int i;

    for (i = 0; i < g_child_count; i++) {
        child_info_t *c = &g_children[i];

        if (c->state == CHILD_RUNNING && c->pid > 0) {
            if (kill(c->pid, SIGKILL) == 0)
                pm_log("SIGKILL_SENT", "%s (PID: %d) kapanmadi, SIGKILL gonderildi",
                       c->name, (int)c->pid);
            else
                pm_log("ERROR", "%s (PID: %d) icin kill(SIGKILL) basarisiz: %s",
                       c->name, (int)c->pid, strerror(errno));
        }
    }
}

int pm_active_count(void)
{
    int i;
    int count = 0;

    for (i = 0; i < g_child_count; i++) {
        if (g_children[i].state == CHILD_RUNNING)
            count++;
    }
    return count;
}

void pm_report_all_status(void)
{
    /* TODO (commit 3): SIGUSR1 raporu icin her alt surecin durumunu logla */
}

const char *pm_state_str(child_state_t state)
{
    switch (state) {
    case CHILD_NOT_STARTED: return "NOT_STARTED";
    case CHILD_RUNNING:     return "RUNNING";
    case CHILD_EXITED_OK:   return "EXITED_OK";
    case CHILD_STOPPED:     return "STOPPED";
    case CHILD_FAILED:      return "FAILED";
    default:                return "UNKNOWN";
    }
}
