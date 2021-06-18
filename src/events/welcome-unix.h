#ifndef WELCOME_UNIX_H
#define WELCOME_UNIX_H

__SWIRC_BEGIN_DECLS
bool	event_welcome_is_signaled(void);
void	event_welcome_cond_init(void);
void	event_welcome_cond_destroy(void);
void	event_welcome_signalit(void);
__SWIRC_END_DECLS

#endif
