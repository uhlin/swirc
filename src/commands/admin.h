#ifndef CMDS_ADMIN_H
#define CMDS_ADMIN_H

__SWIRC_BEGIN_DECLS
void	cmd_die(const char *);
void	cmd_gline(const char *);
void	cmd_kline(const char *);
void	cmd_rehash(const char *);
void	cmd_restart(const char *);
void	cmd_wallops(const char *);
__SWIRC_END_DECLS

#endif
