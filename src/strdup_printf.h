#ifndef STRDUP_PRINTF_H
#define STRDUP_PRINTF_H

/*lint -printf(1, Strdup_printf) */

char	*Strdup_printf  (const char *fmt, ...) PRINTFLIKE(1);
char	*Strdup_vprintf (const char *fmt, va_list);

#endif
