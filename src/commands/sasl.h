#ifndef CMD_SASL_H
#define CMD_SASL_H

#include "../textBuffer.h"

/*lint -sem(get_list_of_matching_sasl_cmds, r_null) */
/*lint -sem(solve_ecdsa_nist256p_challenge, r_null) */

__SWIRC_BEGIN_DECLS
extern const char	g_decrypted_pass_sym;
extern const char	g_encrypted_pass_sym;
extern const char	g_unencrypted_pass_sym;

extern const char g_sasl_pass_allowed_chars[];

void		 cmd_sasl(const char *);
PTEXTBUF	 get_list_of_matching_sasl_cmds(const char *);
char		*solve_ecdsa_nist256p_challenge(const char *, char **);
__SWIRC_END_DECLS

#endif
