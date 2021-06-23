/* filePred.c  --  File Predicates
   Copyright (C) 2012-2021 Markus Uhlin. All rights reserved.

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

#include "filePred.h"

#if WIN32
#define S_ISDIR(m)	((m & _S_IFMT) == _S_IFDIR)
#define S_ISCHR(m)	((m & _S_IFMT) == _S_IFCHR)
#define S_ISREG(m)	((m & _S_IFMT) == _S_IFREG)

#define stat _stat
#endif /* WIN32 */

bool
file_exists(const char *path)
{
	struct stat sb = { 0 };

	return (path != NULL && *path != '\0' && stat(path, &sb) == 0);
}

bool
is_directory(const char *path)
{
	struct stat sb = { 0 };

	if (path == NULL || *path == '\0')
		return false;
	return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

/* character special file */
bool
is_device(const char *path)
{
	struct stat sb = { 0 };

	if (path == NULL || *path == '\0')
		return false;
	return (stat(path, &sb) == 0 && S_ISCHR(sb.st_mode));
}

bool
is_regular_file(const char *path)
{
	struct stat sb = { 0 };

	if (path == NULL || *path == '\0')
		return false;
	return (stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}
