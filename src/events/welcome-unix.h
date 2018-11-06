#ifndef WELCOME_UNIX_H
#define WELCOME_UNIX_H

#ifdef __cplusplus
extern "C" {
#endif

bool event_welcome_is_signaled  (void);
void event_welcome_cond_init    (void);
void event_welcome_cond_destroy (void);
void event_welcome_signalit     (void);

#ifdef __cplusplus
}
#endif

#endif
