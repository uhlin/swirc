#ifndef SPELL_H
#define SPELL_H

__SWIRC_BEGIN_DECLS
void spell_init(void);
void spell_deinit(void);

bool spell_word(const char *);
bool spell_wide_word(const wchar_t *);
__SWIRC_END_DECLS

#endif
