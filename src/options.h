#ifndef OPTIONS_H
#define OPTIONS_H

enum {
	UNRECOGNIZED_OPTION = '?',
	OPTION_ARG_MISSING = ':'
};

__SWIRC_BEGIN_DECLS
extern int	 g_option_index;
extern int	 g_option_save;
extern char	*g_option_arg;

int	options(int, char *[], const char *);
__SWIRC_END_DECLS

#endif
