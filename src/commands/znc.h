#ifndef CMD_ZNC_H
#define CMD_ZNC_H

#include "../textBuffer.h"

__SWIRC_BEGIN_DECLS
extern volatile bool g_invoked_by_znc_jump_net;

void cmd_znc(CSTRING);

PTEXTBUF get_list_of_matching_znc_commands(CSTRING);
__SWIRC_END_DECLS

#endif
