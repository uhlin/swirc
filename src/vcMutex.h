#ifndef VC_MUTEX_H
#define VC_MUTEX_H

#include <windows.h>

/*lint -sem(mutex_lock, thread_lock) */
/*lint -sem(mutex_unlock, thread_unlock) */

__SWIRC_BEGIN_DECLS
void	mutex_lock(HANDLE *);
void	mutex_unlock(HANDLE *);
void	mutex_destroy(HANDLE *);
void	mutex_new(HANDLE *);
__SWIRC_END_DECLS

#endif
