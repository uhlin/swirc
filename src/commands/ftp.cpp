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
