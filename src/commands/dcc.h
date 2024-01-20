#ifndef CMD_DCC_H
#define CMD_DCC_H

#include <openssl/ssl.h>

__SWIRC_BEGIN_DECLS
void cmd_dcc(const char *);
__SWIRC_END_DECLS

#ifdef __cplusplus
namespace dcc
{
	void init(void);
	void deinit(void);
	void handle_incoming_conn(SSL *);
}
#endif

#endif
