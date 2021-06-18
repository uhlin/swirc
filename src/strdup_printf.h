#ifndef STRDUP_PRINTF_H
#define STRDUP_PRINTF_H

/*lint -printf(1, strdup_printf) */

__SWIRC_BEGIN_DECLS
char	*strdup_printf(const char *, ...) PRINTFLIKE(1);
char	*strdup_vprintf(const char *, va_list);
__SWIRC_END_DECLS

#endif
