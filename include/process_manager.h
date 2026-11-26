#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

/* ---- Ayarlar ---- */
#define PM_MAX_CHILDREN        8                  /* tablodaki en fazla alt surec  */
#define PM_MAX_RESTARTS        3                  /* alt surec basina restart limiti */
#define PM_CHECK_INTERVAL_SEC  10                 /* SIGALRM periyodik kontrol (sn) */
#define PM_WORKER_PATH         "./workers/worker" /* exec edilecek program          */

/* ---- Alt surec durumlari ---- */
typedef enum {
    CHILD_NOT_STARTED = 0, /* tabloda var, henuz fork edilmedi              */
    CHILD_RUNNING,         /* calisiyor                                      */
    CHILD_EXITED_OK,       /* exit(0) ile normal bitti -> restart yapilmaz   */
    CHILD_STOPPED,         /* kapanis sirasinda watchdog tarafindan durdu    */
    CHILD_FAILED           /* restart limiti asildi -> vazgecildi            */
} child_state_t;

/* ---- Kurulum ---- */
int  pm_init(void);                 /* tabloyu doldurur, periyodik timer'i kurar */
int  pm_start_all(void);            /* tum alt surecleri fork+exec ile baslatir  */

/* ---- Ana donguden cagrilanlar ---- */
void pm_handle_sigchld(void);       /* g_child_exited set olunca: waitpid + restart */
void pm_periodic_check(void);       /* g_alarm_triggered set olunca               */

/* ---- Kapanis destegi ---- */
void pm_stop_restarts(void);        /* restart'i ve periyodik timer'i durdurur   */
void pm_send_sigterm_all(void);     /* calisan tum alt sureclere SIGTERM         */
void pm_force_kill_remaining(void); /* hala calisanlara SIGKILL                  */
int  pm_active_count(void);         /* calisan alt surec sayisi                  */

/* ---- Raporlama ---- */
void        pm_report_all_status(void);      /* SIGUSR1 raporu icin (logger.c cagirir) */
const char *pm_state_str(child_state_t state);

#endif /* PROCESS_MANAGER_H */
