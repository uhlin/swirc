#ifndef STRDUP_PRINTF_H
#define STRDUP_PRINTF_H

/*lint -printf(1, strdup_printf) */

__SWIRC_BEGIN_DECLS
STRING	strdup_printf(CSTRING, ...) PRINTFLIKE(1);
STRING	strdup_vprintf(CSTRING, va_list);
__SWIRC_END_DECLS

#endif
