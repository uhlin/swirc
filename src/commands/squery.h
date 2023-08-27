#ifndef CMD_SQUERY_H
#define CMD_SQUERY_H

__SWIRC_BEGIN_DECLS

void cmd_squery(const char *) NONNULL;

//lint -sem(get_list_of_matching_squery_commands, r_null)
PTEXTBUF get_list_of_matching_squery_commands(CSTRING);

__SWIRC_END_DECLS

#endif
