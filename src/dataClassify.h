#ifndef DATA_CLASSIFY_H
#define DATA_CLASSIFY_H

__SWIRC_BEGIN_DECLS
bool	is_alphabetic(const char *);
bool	is_numeric(const char *);
bool	is_whiteSpace(const char *);
bool	is_irc_channel(const char *);
bool	is_valid_nickname(const char *);
bool	is_valid_username(const char *);
bool	is_valid_real_name(const char *);
bool	is_valid_hostname(const char *);
__SWIRC_END_DECLS

/* Inline function definitions
   =========================== */

#ifndef _lint
static SW_INLINE bool
isNull(const void *data)
{
	return (data == NULL);
}
#else
#define isNull(_obj) ((_obj) == NULL)
#endif

static SW_INLINE bool
isEmpty(const char *data)
{
	return (*data == '\0');
}

#include <ctype.h>
#include <limits.h>
#include <stdio.h>		/* EOF */

static SW_INLINE int
sw_isalnum(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isalnum(c));
}

static SW_INLINE int
sw_isalpha(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isalpha(c));
}

static SW_INLINE int
sw_isdigit(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isdigit(c));
}

static SW_INLINE int
sw_isspace(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isspace(c));
}

static SW_INLINE int
sw_isprint(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isprint(c));
}

static SW_INLINE int
sw_isupper(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isupper(c));
}

static SW_INLINE int
sw_islower(int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : islower(c));
}

#endif /* DATA_CLASSIFY_H */
