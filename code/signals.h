#ifndef SIGNALS_H
#define SIGNALS_H

/*
 * Configures the shell's signal handlers.
 */
void setup_shell_signals(void);

/*
 * Restores default signal handling inside child processes.
 */
void restore_default_signals(void);

/*
 * Reap any remaining zombie processes.
 * Call this periodically as a safety net.
 */
void reap_zombies(void);

#endif