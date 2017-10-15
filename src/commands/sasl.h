#ifndef CMD_SASL_H
#define CMD_SASL_H

/*lint -sem(solve_ecdsa_nist256p_challenge, r_null) */

char	*solve_ecdsa_nist256p_challenge(const char *, char **);
void	 cmd_sasl(const char *);

#endif
