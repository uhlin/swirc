/* ossl-scripts.c
   Copyright (C) 2021 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include "common.h"

#include <sys/stat.h>

#include "errHand.h"
#include "filePred.h"
#include "libUtils.h"
#include "nestHome.h"
#include "ossl-scripts.h"
#include "strdup_printf.h"
#include "tls-server.h"

#if defined(UNIX)
#define SLASH "/"
#elif defined(WIN32)
#define SLASH "\\"
#endif

/*lint -e786 */

static const char  root_ca_script_title[] = "1-root-ca";
static const char *root_ca_script_lines[] = {
	SCR_SHEBANG,
	SCR_COMMENT " Create the root CA",
	"",
	"openssl req -newkey rsa:2048 -sha256 -keyout rootkey.pem -out "
	"rootreq.pem",
	"",
	"openssl x509 -req -in rootreq.pem -sha256 -extfile " EXTFILE
	" -extensions v3_ca -signkey rootkey.pem -out rootcert.pem",
	"",
	CAT_CMD " rootcert.pem > " ROOT_PEM,
	CAT_CMD " rootkey.pem >> " ROOT_PEM,
	"",
	"openssl x509 -subject -issuer -noout -in " ROOT_PEM,
};

static const char  server_ca_script_title[] = "2-server-ca";
static const char *server_ca_script_lines[] = {
	SCR_SHEBANG,
	SCR_COMMENT " Create the server CA (and sign it with the root CA)",
	"",
	"openssl req -newkey rsa:2048 -sha256 -keyout serverCAkey.pem -out "
	"serverCAreq.pem",
	"",
	"openssl x509 -req -in serverCAreq.pem -sha256 -extfile " EXTFILE
	" -extensions v3_ca"
	" -CA " ROOT_PEM
	" -CAkey " ROOT_PEM
	" -CAcreateserial -out serverCAcert.pem",
	"",
	CAT_CMD " serverCAcert.pem > " SERVER_CA_PEM,
	CAT_CMD " serverCAkey.pem >> " SERVER_CA_PEM,
	CAT_CMD " rootcert.pem >> " SERVER_CA_PEM,
	"",
	"openssl x509 -subject -issuer -noout -in " SERVER_CA_PEM,
};

static const char  server_cert_script_title[] = "3-server-cert";
static const char *server_cert_script_lines[] = {
	SCR_SHEBANG,
	SCR_COMMENT " Create the server's certificate "
	"(and sign it with the server CA)",
	"",
	"openssl req -newkey rsa:2048 -sha256 -keyout serverkey.pem -out "
	"serverreq.pem -nodes",
	"",
	"openssl x509 -req -in serverreq.pem -sha256 -extfile " EXTFILE
	" -extensions usr_cert"
	" -CA " SERVER_CA_PEM
	" -CAkey " SERVER_CA_PEM
	" -CAcreateserial -out servercert.pem",
	"",
	CAT_CMD " servercert.pem > " SERVER_PEM,
	CAT_CMD " serverkey.pem >> " SERVER_PEM,
	CAT_CMD " serverCAcert.pem >> " SERVER_PEM,
	CAT_CMD " rootcert.pem >> " SERVER_PEM,
	"",
	"openssl x509 -subject -issuer -noout -in " SERVER_PEM,
};

static const char  client_cert_script_title[] = "4-client-cert";
static const char *client_cert_script_lines[] = {
	SCR_SHEBANG,
	SCR_COMMENT " Create the client certificate "
	"(and sign it with the root CA)",
	"",
	"openssl req -newkey rsa:2048 -sha256 -keyout clientkey.pem -out "
	"clientreq.pem -nodes",
	"",
	"openssl x509 -req -in clientreq.pem -sha256 -extfile " EXTFILE
	" -extensions usr_cert"
	" -CA " ROOT_PEM
	" -CAkey " ROOT_PEM
	" -CAcreateserial -out clientcert.pem",
	"",
	CAT_CMD " clientcert.pem > " CLIENT_PEM,
	CAT_CMD " clientkey.pem >> " CLIENT_PEM,
	CAT_CMD " rootcert.pem >> " CLIENT_PEM,
	"",
	"openssl x509 -subject -issuer -noout -in " CLIENT_PEM,
};

static const char  dhparams_script_title[] = "5-dhparams";
static const char *dhparams_script_lines[] = {
	SCR_SHEBANG,
	SCR_COMMENT " Create DH parameter files "
	"(may take a long time)",
	"",
	"openssl dhparam -outform PEM -out " DH_PEM1 " -check -text -5 2048",
	"openssl dhparam -outform PEM -out " DH_PEM2 " -check -text -5 4096",
};

/*lint +e786 */

static void
create_doit(const char *title, const char *lines[], const size_t size)
{
	FILE	*fp = NULL;
	char	*path;

	if (g_home_dir == NULL)
		return;

	path = strdup_printf("%s%s%s%s", g_home_dir, SLASH, title, SCR_SUFFIX);

	if (file_exists(path) || (fp = xfopen(path, "w+")) == NULL)
		goto err;
	for (size_t i = 0; i < size; i++)
		(void) fprintf(fp, "%s\n", lines[i]);
	(void) fclose(fp);
	fp = NULL;

#ifdef UNIX
	errno = 0;

	if (chmod(path, S_IRWXU) != 0)
		err_log(errno, "create_doit: chmod");
#endif

	free(path);
	return;

  err:
	if (fp)
		(void) fclose(fp);
	free(path);
}

/*
 * Create the root CA
 */
void
create_root_ca_script(void)
{
	create_doit(root_ca_script_title, root_ca_script_lines,
	    ARRAY_SIZE(root_ca_script_lines));
}

/*
 * Create the server CA (and sign it with the root CA)
 */
void
create_server_ca_script(void)
{
	create_doit(server_ca_script_title, server_ca_script_lines,
	    ARRAY_SIZE(server_ca_script_lines));
}

/*
 * Create the server's certificate (and sign it with the server CA)
 */
void
create_server_cert_script(void)
{
	create_doit(server_cert_script_title, server_cert_script_lines,
	    ARRAY_SIZE(server_cert_script_lines));
}

/*
 * Create the client certificate (and sign it with the root CA)
 */
void
create_client_cert_script(void)
{
	create_doit(client_cert_script_title, client_cert_script_lines,
	    ARRAY_SIZE(client_cert_script_lines));
}

/*
 * Create DH parameter files
 */
void
create_dhparams_script(void)
{
	create_doit(dhparams_script_title, dhparams_script_lines,
	    ARRAY_SIZE(dhparams_script_lines));
}
