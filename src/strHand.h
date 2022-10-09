#ifndef STRING_HANDLING_H
#define STRING_HANDLING_H

#include <stdint.h> /* SIZE_MAX */

#ifndef STRINGS_MATCH
#define STRINGS_MATCH 0
#endif

/*lint -sem(strColor, pure) */
/*lint -sem(strcasestr, r_null) */
/*lint -printf(3, sw_snprintf) */

__SWIRC_BEGIN_DECLS
char	*strToLower(char *);
char	*strToUpper(char *);
char	*sw_strdup(const char *);
char	*trim(char *);
const char *
	 strColor(short int color) NO_SIDE_EFFECT;
int	 strFeed(char *string, int count);
#if defined(HAVE_STRCASESTR) && HAVE_STRCASESTR == 0
char	*strcasestr(const char *, const char *);
#endif
int	 sw_strcat(char *dest, const char *src, size_t);
int	 sw_strcpy(char *dest, const char *src, size_t);
int	 sw_wcscat(wchar_t *dest, const wchar_t *src, size_t);
int	 sw_wcscpy(wchar_t *dest, const wchar_t *src, size_t);
size_t	 xstrnlen(const char *, size_t);
void	 squeeze(char *, const char *);
void	 sw_snprintf(char *, size_t, const char *, ...) PRINTFLIKE(3);
__SWIRC_END_DECLS

/* Inline function definitions
   =========================== */

#if defined(LINUX) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <string.h>

#ifdef UNIX
#include <strings.h>
#endif

static SW_INLINE bool
strings_match(const char *s1, const char *s2)
{
	return (strcmp(s1, s2) == 0);
}

static SW_INLINE bool
strings_match_ignore_case(const char *s1, const char *s2)
{
#if defined(UNIX)
	return (strcasecmp(s1, s2) == 0);
#elif defined(WIN32)
	return (_stricmp(s1, s2) == 0);
#endif
}

#endif
