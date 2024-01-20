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

	const char *get_upload_dir(void);
	void handle_incoming_conn(SSL *);
	bool want_unveil_uploads(void);
}
#endif

#endif
