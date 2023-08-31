#ifndef CMD_SERVICES_H
#define CMD_SERVICES_H

__SWIRC_BEGIN_DECLS
void	cmd_chanserv(CSTRING);
void	cmd_nickserv(CSTRING);
void	cmd_qbot(CSTRING);

//lint -sem(get_list_of_matching_cs_cmds, r_null)
//lint -sem(get_list_of_matching_ns_cmds, r_null)
PTEXTBUF get_list_of_matching_cs_cmds(CSTRING);
PTEXTBUF get_list_of_matching_ns_cmds(CSTRING);
__SWIRC_END_DECLS

#endif
