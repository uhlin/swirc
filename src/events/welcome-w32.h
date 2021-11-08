#ifndef WELCOME_W32_H
#define WELCOME_W32_H

#include <windows.h> /* MAXDWORD */

#ifndef MAXDWORD
#define MAXDWORD 4294967295
#endif

__SWIRC_BEGIN_DECLS
bool	event_welcome_is_signaled(void);
void	event_welcome_cond_init(void);
void	event_welcome_cond_destroy(void);
void	event_welcome_signalit(void);
__SWIRC_END_DECLS

#endif
