#ifndef THEME_H
#define THEME_H

#include "int_unparse.h"

typedef struct tagTHEME_HTBL_ENTRY {
    char *name;
    char *value;
    struct tagTHEME_HTBL_ENTRY *next;
} THEME_HTBL_ENTRY, *PTHEME_HTBL_ENTRY;

__SWIRC_BEGIN_DECLS
void	theme_init(void);
void	theme_deinit(void);

/*lint -sem(Theme_mod, r_null) */

bool		 theme_bool_unparse(const char *, bool);
char		*Theme_mod(const char *);
const char	*Theme(const char *);
int		 theme_item_install(const char *name, const char *value);
int		 theme_item_undef(const char *name);
long int	 theme_integer_unparse(struct integer_unparse_context *);
short int	 theme_color_unparse(const char *, short int);
void		 theme_create(const char *path, const char *mode);
void		 theme_do_save(const char *path, const char *mode);
void		 theme_readit(const char *path, const char *mode);
__SWIRC_END_DECLS

#define COLOR1		Theme("primary_color")
#define COLOR2		Theme("secondary_color")
#define COLOR3		Theme("color3")
#define COLOR4		Theme("color4")
#define GFX_FAILURE	Theme("gfx_failure")
#define GFX_SUCCESS	Theme("gfx_success")
#define GFX_WARN	Theme("gfx_warning")
#define LEFT_BRKT	Theme("left_bracket")
#define RIGHT_BRKT	Theme("right_bracket")
#define THE_SPEC1	Theme("specifier1")
#define THE_SPEC2	Theme("specifier2")
#define THE_SPEC3	Theme("specifier3")

#endif
