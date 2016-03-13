#ifndef THEME_H
#define THEME_H

#include "int_unparse.h"

typedef struct tagTHEME_HTBL_ENTRY {
    char *name;
    char *value;
    struct tagTHEME_HTBL_ENTRY *next;
} THEME_HTBL_ENTRY, *PTHEME_HTBL_ENTRY;

/*lint -sem(Theme_mod, r_null) */

bool		 theme_bool_unparse    (const char *item_name, bool fallback_default);
char		*Theme_mod             (const char *item_name);
const char	*Theme                 (const char *item_name);
int		 theme_item_install    (const char *name, const char *value);
int		 theme_item_undef      (const char *name);
long int	 theme_integer_unparse (struct integer_unparse_context *);
short int	 theme_color_unparse   (const char *item_name, short int fallback_color);
void		 theme_create          (const char *path, const char *mode);
void		 theme_deinit          (void);
void		 theme_do_save         (const char *path, const char *mode);
void		 theme_init            (void);
void		 theme_readit          (const char *path, const char *mode);

#define COLOR1		Theme("primary_color")
#define COLOR2		Theme("secondary_color")
#define GFX_FAILURE	Theme("gfx_failure")
#define GFX_SUCCESS	Theme("gfx_success")
#define GFX_WARN	Theme("gfx_warning")
#define LEFT_BRKT	Theme("left_bracket")
#define RIGHT_BRKT	Theme("right_bracket")
#define THE_SPEC1	Theme("specifier1")
#define THE_SPEC2	Theme("specifier2")
#define THE_SPEC3	Theme("specifier3")

#endif
