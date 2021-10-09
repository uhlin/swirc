#ifndef CMD_IGNORE_H
#define CMD_IGNORE_H

__SWIRC_BEGIN_DECLS
void	cmd_ignore(const char *) PTR_ARGS_NONNULL;
void	cmd_unignore(const char *) PTR_ARGS_NONNULL;

bool	is_valid_regex(const char *, char **) PTR_ARGS_NONNULL;
__SWIRC_END_DECLS

#endif
