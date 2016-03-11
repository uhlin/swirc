#ifndef OPTIONS_H
#define OPTIONS_H

enum {
    UNRECOGNIZED_OPTION = '?',
    OPTION_ARG_MISSING  = ':'
};

extern int	 g_option_index;
extern int	 g_option_save;
extern char	*g_option_arg;

int options(int argc, char *argv[], const char *optstring);

#endif
