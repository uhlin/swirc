#ifndef NICKLIST_H
#define NICKLIST_H

#include "window.h"

__SWIRC_BEGIN_DECLS
extern const int	g_nicklist_maxnick;
extern const int	g_nicklist_scroll_amount;

int	nicklist_new(PIRC_WINDOW);
int	nicklist_destroy(PIRC_WINDOW);

int	nicklist_draw(PIRC_WINDOW, const int);
int	nicklist_get_width(const PIRC_WINDOW);

void	nicklist_scroll_down(PIRC_WINDOW);
void	nicklist_scroll_up(PIRC_WINDOW);
int	nicklist_update(PIRC_WINDOW);
__SWIRC_END_DECLS

#endif
