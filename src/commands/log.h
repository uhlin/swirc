#ifndef SRC_COMMANDS_LOG_H_
#define SRC_COMMANDS_LOG_H_

#include "../textBuffer.h"

//lint -sem(get_list_of_matching_log_cmds, r_null)

__SWIRC_BEGIN_DECLS
void cmd_log(CSTRING);
PTEXTBUF get_list_of_matching_log_cmds(CSTRING) NONNULL;
__SWIRC_END_DECLS

#endif
