#ifndef CMD_DCC_H
#define CMD_DCC_H

#include <openssl/ssl.h>

__SWIRC_BEGIN_DECLS
void cmd_dcc(const char *);
void dcc_init(void);
void dcc_deinit(void);
void dcc_handle_incoming_conn(SSL *);
__SWIRC_END_DECLS

#endif
