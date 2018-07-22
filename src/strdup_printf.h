#ifndef STRDUP_PRINTF_H
#define STRDUP_PRINTF_H

/*lint -printf(1, strdup_printf) */

#ifdef __cplusplus
extern "C" {
#endif

char	*strdup_printf  (const char *fmt, ...) PRINTFLIKE(1);
char	*strdup_vprintf (const char *fmt, va_list);

#ifdef __cplusplus
}
#endif

#endif
