#ifndef SPELL_H
#define SPELL_H

__SWIRC_BEGIN_DECLS
extern const char g_aff_suffix[];
extern const char g_dic_suffix[];

void spell_init(void);
void spell_deinit(void);

bool spell_word(const char *);
bool spell_wide_word(const wchar_t *);
__SWIRC_END_DECLS

#endif
