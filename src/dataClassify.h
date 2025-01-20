#ifndef DATA_CLASSIFY_H
#define DATA_CLASSIFY_H

#include <stdint.h>

#define TEST_XWCWIDTH 0

typedef struct tagRANGE {
#if defined(UNIX)
	wchar_t start;
	wchar_t stop;
#elif defined(WIN32)
	uint32_t start;
	uint32_t stop;
#endif

	CSTRING comment;
} RANGE, *PRANGE;

__SWIRC_BEGIN_DECLS
bool	is_alphabetic(const char *);
bool	is_cjk(const wchar_t);
bool	is_combined(const wchar_t);
bool	is_irc_channel(const char *);
bool	is_numeric(const char *);
bool	is_printable(const char *);
bool	is_valid_filename(const char *);
bool	is_valid_hostname(const char *);
bool	is_valid_nickname(const char *);
bool	is_valid_real_name(const char *);
bool	is_valid_username(const char *);
bool	is_whitespace(const char *);

int	xwcwidth(const wchar_t, const int);
int	xwcswidth(const wchar_t *, const int) NONNULL;
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

#ifndef _lint
static SW_INLINE bool
isValid(const void *ptr)
{
#if defined(HAVE_ETEXT_SEGMENT)
	extern char etext;
	return (ptr != NULL && ((const char *) ptr) > &etext);
#else
	return (ptr != NULL);
#endif
}
#else
#define isValid(_ptr) ((_ptr) != NULL)
#endif

#include <ctype.h>
#include <limits.h>
#include <stdio.h>		/* EOF */

static SW_INLINE int
sw_isalnum(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isalnum(c));
}

static SW_INLINE int
sw_isalpha(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isalpha(c));
}

static SW_INLINE int
sw_isdigit(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isdigit(c));
}

static SW_INLINE int
sw_isspace(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isspace(c));
}

static SW_INLINE int
sw_isprint(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isprint(c));
}

static SW_INLINE int
sw_isupper(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : isupper(c));
}

static SW_INLINE int
sw_islower(const int c)
{
	return ((c == EOF || c < 0 || c > UCHAR_MAX) ? false : islower(c));
}

#endif /* DATA_CLASSIFY_H */
