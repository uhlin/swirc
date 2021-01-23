#ifndef TITLEBAR_H
#define TITLEBAR_H

/*lint -printf(1, titlebar) */

__SWIRC_BEGIN_DECLS
void	titlebar(const char *, ...) PRINTFLIKE(1);
void	titlebar_deinit(void);
void	titlebar_init(void);
void	titlebar_recreate(int cols);
__SWIRC_END_DECLS

#endif
