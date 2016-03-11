#ifndef PTHREAD_MUTEX_H
#define PTHREAD_MUTEX_H

#include <pthread.h>

/*lint -sem(mutex_lock, thread_lock) */
/*lint -sem(mutex_unlock, thread_unlock) */

void	mutex_lock    (pthread_mutex_t *);
void	mutex_unlock  (pthread_mutex_t *);
void	mutex_destroy (pthread_mutex_t *);
void	mutex_new     (pthread_mutex_t *);

#endif
