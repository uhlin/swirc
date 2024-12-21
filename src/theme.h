#ifndef SRC_THEME_H_
#define SRC_THEME_H_

#include "integer-context.h"

typedef struct tagTHEME_HTBL_ENTRY {
	char *name;
	char *value;
	struct tagTHEME_HTBL_ENTRY *next;
} THEME_HTBL_ENTRY, *PTHEME_HTBL_ENTRY;

__SWIRC_BEGIN_DECLS
void	theme_init(void);
void	theme_deinit(void);

/*lint -sem(Theme_mod, r_null) */

bool		 theme_bool(const char *, bool);
char		*Theme_mod(const char *);
const char	*Theme(const char *);
errno_t		 theme_item_install(const char *name, const char *value);
errno_t		 theme_item_undef(const char *name);
long int	 theme_integer(const struct integer_context *);
short int	 theme_color(const char *, short int);
void		 theme_create(const char *path, const char *mode);
void		 theme_do_save(const char *path, const char *mode);
void		 theme_readit(const char *path, const char *mode);
__SWIRC_END_DECLS

#define COLOR_device	"\00308"
#define COLOR_directory "\00312"
#define COLOR_exec	"\00309"
#define COLOR_fifo	"\00307"
#define COLOR_socket	"\00313"
#define COLOR_symlink	"\00311"

#define SYM_exec	"*"
#define SYM_fifo	"|"
#define SYM_socket	"="
#define SYM_symlink	"@"

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
