#ifndef STRING_HANDLING_H
#define STRING_HANDLING_H

#ifndef STRINGS_MATCH
#define STRINGS_MATCH 0
#endif

/*lint -sem(strColor, pure) */
/*lint -printf(3, sw_snprintf) */

#ifdef __cplusplus
extern "C" {
#endif

char	*strToLower(char *);
char	*strToUpper(char *);
char	*sw_strdup(const char *string);
char	*trim(char *string);
const char *
	 strColor(short int color) NO_SIDE_EFFECT;
int	 strFeed(char *string, int count);
int	 sw_strcat(char *dest, const char *src, size_t);
int	 sw_strcpy(char *dest, const char *src, size_t);
int	 sw_wcscat(wchar_t *dest, const wchar_t *src, size_t);
int	 sw_wcscpy(wchar_t *dest, const wchar_t *src, size_t);
void	 squeeze(char *buffer, const char *rej);
void	 sw_snprintf(char *dest, size_t, const char *fmt, ...) PRINTFLIKE(3);

#ifdef __cplusplus
}
#endif

/* Inline function definitions
   =========================== */

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
