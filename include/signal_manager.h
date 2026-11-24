#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

extern volatile sig_atomic_t g_shutdown_requested;
extern volatile sig_atomic_t g_shutdown_signal;
extern volatile sig_atomic_t g_status_requested;
extern volatile sig_atomic_t g_child_exited;
extern volatile sig_atomic_t g_alarm_triggered;

int signal_manager_init(void);

const char *signal_manager_get_name(int signum);

#endif

