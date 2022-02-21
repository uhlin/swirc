#include "common.h"

#ifdef WIN32
#include "../DesktopNotificationManagerCompat.hpp"
#include "../ToastsAPI.hpp"
#endif

#include "../errHand.h"
#include "../printtext.h"
#include "../strHand.h"

#include "cleartoasts.h"

/*
 * usage: /cleartoasts
 */
void
cmd_cleartoasts(const char *data)
{
	if (!strings_match(data, "")) {
		print_and_free("/cleartoasts: implicit trailing data", NULL);
		return;
	}

#if defined(UNIX)
	debug("cmd_cleartoasts() called (currently a no-op)");
#elif defined(WIN32)
	Toasts::ClearToasts();
#endif
}
