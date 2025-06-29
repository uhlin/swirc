#ifndef STATUSBAR_H
#define STATUSBAR_H

__SWIRC_BEGIN_DECLS
extern char g_user_modes[100];

void	statusbar_init(void);
void	statusbar_deinit(void);

void	statusbar_show(void);
void	statusbar_hide(void);

void	statusbar_recreate(int rows, int cols);
void	statusbar_top_panel(void);
void	statusbar_update(void);
__SWIRC_END_DECLS

#endif
