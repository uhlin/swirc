#ifndef OPENSSL_SCRIPTS_H
#define OPENSSL_SCRIPTS_H

#if defined(UNIX)
#define CAT_CMD		"cat"
#define EXTFILE		"/etc/ssl/openssl.cnf"
#define SCR_COMMENT	"#"
#define SCR_SHEBANG	"#!/bin/sh"
#define SCR_SUFFIX	".sh"
#elif defined(WIN32)
#define CAT_CMD		"type"
#define EXTFILE		"openssl.cnf"
#define SCR_COMMENT	"::"
#define SCR_SHEBANG	""
#define SCR_SUFFIX	".bat"
#endif

__SWIRC_BEGIN_DECLS
void	create_root_ca_script(void);
void	create_server_ca_script(void);
void	create_server_cert_script(void);
void	create_client_cert_script(void);
__SWIRC_END_DECLS

#endif
