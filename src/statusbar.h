#ifndef STATUSBAR_H
#define STATUSBAR_H

void	statusbar_deinit(void);
void	statusbar_hide(void);
void	statusbar_init(void);
void	statusbar_recreate(int rows, int cols);
void	statusbar_show(void);
void	statusbar_update_display_beta(void);

#endif
