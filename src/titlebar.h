#ifndef TITLEBAR_H
#define TITLEBAR_H

/*lint -printf(1, titlebar) */

void	titlebar(const char *fmt, ...);
void	titlebar_deinit(void);
void	titlebar_init(void);
void	titlebar_recreate(int cols);

#endif
