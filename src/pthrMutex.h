#ifndef PTHREAD_MUTEX_H
#define PTHREAD_MUTEX_H

#include <pthread.h>

/*lint -sem(mutex_lock, thread_lock) */
/*lint -sem(mutex_unlock, thread_unlock) */

__SWIRC_BEGIN_DECLS
void	mutex_lock(pthread_mutex_t *);
void	mutex_unlock(pthread_mutex_t *);
void	mutex_destroy(pthread_mutex_t *);
void	mutex_new(pthread_mutex_t *);
__SWIRC_END_DECLS

#endif
