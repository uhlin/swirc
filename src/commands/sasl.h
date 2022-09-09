#ifndef CMD_SASL_H
#define CMD_SASL_H

/*lint -sem(solve_ecdsa_nist256p_challenge, r_null) */

__SWIRC_BEGIN_DECLS
extern const char	g_decrypted_pass_sym;
extern const char	g_encrypted_pass_sym;
extern const char	g_unencrypted_pass_sym;

char	*solve_ecdsa_nist256p_challenge(const char *, char **);
void	 cmd_sasl(const char *);
__SWIRC_END_DECLS

#endif
