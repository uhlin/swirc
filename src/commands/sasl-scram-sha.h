#ifndef SASL_SCRAM_SHA_H
#define SASL_SCRAM_SHA_H

__SWIRC_BEGIN_DECLS
extern volatile bool	g_sasl_scram_sha_got_first_msg;

int	sasl_scram_sha_send_client_first_msg(void);
int	sasl_scram_sha_send_client_final_msg(CSTRING proof);

int	sasl_scram_sha_handle_serv_first_msg(CSTRING);
int	sasl_scram_sha_handle_serv_final_msg(CSTRING);
__SWIRC_END_DECLS

#endif
