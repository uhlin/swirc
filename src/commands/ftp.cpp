/* The FTP command
   Copyright (C) 2024, 2025 Markus Uhlin. All rights reserved.

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

#if __OpenBSD__
#include <sys/param.h>
#endif

#include <netdb.h>

#include <stdexcept>

#include "../config.h"
#include "../dataClassify.h"
#include "../errHand.h"
#include "../filePred.h"
#include "../libUtils.h"
#include "../main.h"
#include "../nestHome.h"
#include "../network.h"
#include "../printtext.h"
#include "../strHand.h"
#include "../strdup_printf.h"
#include "../theme.h"

#include "dcc.h" /* list_dir() */
#include "ftp.h"

void
cmd_ftp(CSTRING data)
{
	CSTRING			arg[2];
	CSTRING			subcmd;
	STRING			dcopy;
	STRING			last = const_cast<STRING>("");
	static chararray_t	cmd  = "/ftp";
	static chararray_t	sep  = "\n";

	if (strings_match(data, "")) {
		printtext_print("err", "insufficient args");
		return;
	}

	dcopy = sw_strdup(data);
	(void) strFeed(dcopy, 2);

	if ((subcmd = strtok_r(dcopy, sep, &last)) == nullptr) {
		printf_and_free(dcopy, "%s: insufficient args", cmd);
		return;
	} else if (!subcmd_ok(subcmd)) {
		printf_and_free(dcopy, "%s: invalid subcommand '%s'", cmd,
		    subcmd);
		return;
	}

	arg[0] = strtok_r(nullptr, sep, &last);
	arg[1] = strtok_r(nullptr, sep, &last);

	if (strings_match(subcmd, "exit"))
		subcmd_exit();
	else if (strings_match(subcmd, "login"))
		subcmd_login();
	else if (strings_match(subcmd, "ls"))
		subcmd_ls(arg[0]);
	else
		printtext_print("err", "%s: invalid subcommand", cmd);
	free(dcopy);
}

CSTRING
ftp::get_upload_dir(void)
{
	static CSTRING dir;

	dir = Config("ftp_upload_dir");

#if defined(OpenBSD) && OpenBSD >= 201811
	return (strings_match(dir, "") ? g_ftp_upload_dir : dir);
#else
	if (!is_directory(dir))
		return (g_ftp_upload_dir ? g_ftp_upload_dir : "");
	return dir;
#endif
}

int
ftp::send_printf(SOCKET sock, CSTRING fmt, ...)
{
	char	*buffer;
	int	 n_sent;
	va_list	 ap;

	if (sock == INVALID_SOCKET)
		return -1;
	else if (fmt == nullptr)
		err_exit(EINVAL, "%s", __func__);
	else if (strings_match(fmt, ""))
		return 0;

	va_start(ap, fmt);
	buffer = strdup_vprintf(fmt, ap);
	va_end(ap);

	errno = 0;

#if defined(UNIX)
	if ((n_sent = send(sock, buffer, strlen(buffer), 0)) == -1) {
		free_and_null(&buffer);
		return (errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1);
	}
#elif defined(WIN32)
	if ((n_sent = send(g_socket, buffer, size_to_int(strlen(buffer)), 0)) ==
	    SOCKET_ERROR) {
		free_and_null(&buffer);
		return (WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1);
	}
#endif

	free_and_null(&buffer);
	return n_sent;
}

bool
ftp::want_unveil_uploads(void)
{
	CSTRING		dir = Config("ftp_upload_dir");
	size_t		len1, len2;

	if (strings_match(dir, ""))
		return false;

	len1 = strlen(dir);
	len2 = strlen(g_home_dir);

	if (len1 >= len2 && strncmp(dir, g_home_dir, MIN(len1, len2)) ==
	    STRINGS_MATCH)
		return false;
	return true;
}
