#ifndef INTERNATIONALIZATION_H
#define INTERNATIONALIZATION_H

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#define _(msgid)	gettext(msgid)
#define N_(msgid)	gettext_noop(msgid)
#else
#define _(msgid)	(msgid)
#define N_(msgid)	(msgid)
#endif	/* HAVE_LIBINTL_H */

#endif	/* INTERNATIONALIZATION_H */
