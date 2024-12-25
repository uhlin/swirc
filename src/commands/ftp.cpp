#include "common.h"

#if __OpenBSD__
#include <sys/param.h>
#endif

#include "../config.h"
#include "../filePred.h"
#include "../libUtils.h"
#include "../nestHome.h"
#include "../strHand.h"

#include "ftp.h"

void
cmd_ftp(CSTRING data)
{
	UNUSED_PARAM(data);
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
