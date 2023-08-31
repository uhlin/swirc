#ifndef CMD_SERVICES_H
#define CMD_SERVICES_H

__SWIRC_BEGIN_DECLS
void	cmd_chanserv(const char *);
void	cmd_nickserv(const char *);
void	cmd_qbot(const char *);

//lint -sem(get_list_of_matching_cs_cmds, r_null)
//lint -sem(get_list_of_matching_ns_cmds, r_null)
PTEXTBUF get_list_of_matching_cs_cmds(const char *);
PTEXTBUF get_list_of_matching_ns_cmds(const char *);
__SWIRC_END_DECLS

#endif
