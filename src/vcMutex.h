#ifndef VC_MUTEX_H
#define VC_MUTEX_H

#include <windows.h>

/*lint -sem(mutex_lock, thread_lock) */
/*lint -sem(mutex_unlock, thread_unlock) */

void	mutex_lock    (HANDLE *mutex);
void	mutex_unlock  (HANDLE *mutex);
void	mutex_destroy (HANDLE *mutex);
void	mutex_new     (HANDLE *mutex);

#endif
