#include "common.h"
#ifdef WIN32
#include "../DesktopNotificationManagerCompat.hpp"
#include "../ToastsAPI.hpp"
#endif
#include "cleartoasts.h"

/* usage: /cleartoasts */
void
cmd_cleartoasts(const char *data)
{
#if defined(UNIX)
	(void) 0;
#elif defined(WIN32)
	Toasts::ClearToasts();
#endif
}
