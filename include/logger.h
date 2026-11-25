#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

/* Log dosyasını açar ve loglama sistemini başlatır. Başarılıysa 0, hata varsa -1 döner. */
int logger_init(const char *log_path);

/* Zaman damgalı bir olay kaydını log dosyasına yazar. */
void logger_log_event(const char *event_type, const char *message);

/* Tek bir alt sürecin durum satırını log dosyasına yazar (SIGUSR1 raporlaması için yardımcı). */
void logger_log_process_status(int index, pid_t pid, const char *state, int restart_count);

/* SIGUSR1 geldiğinde main.c tarafından çağrılır; tüm alt süreçlerin durumunu raporlar. */
void logger_report_status(void);

/* Log dosyasını güvenli şekilde kapatır. */
void logger_close(void);

#endif /* LOGGER_H */