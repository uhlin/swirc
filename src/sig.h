#ifndef GUARD_SIG_H
#define GUARD_SIG_H

#define USE_STRSIGNAL 0

__SWIRC_BEGIN_DECLS
void	block_signals(void);
bool	sighand_init(void);
__SWIRC_END_DECLS

#endif
