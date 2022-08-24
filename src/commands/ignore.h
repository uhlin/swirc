#ifndef CMD_IGNORE_H
#define CMD_IGNORE_H

#define MAXIGNORES	101

__SWIRC_BEGIN_DECLS
void	cmd_ignore(const char *) PTR_ARGS_NONNULL;
void	cmd_unignore(const char *) PTR_ARGS_NONNULL;

bool	is_in_ignore_list(const char *, const char *, const char *);
bool	is_valid_regex(const char *, char **) PTR_ARGS_NONNULL;
__SWIRC_END_DECLS

#endif
