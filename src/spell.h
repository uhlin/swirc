#ifndef SPELL_H
#define SPELL_H

#ifdef __cplusplus
class suggestion {
public:
	suggestion();
	suggestion(const char *);
	~suggestion();

	const char *get_word(void);
	const wchar_t *get_wide_word(void);

private:
	char *word;
	wchar_t *wide_word;
};

typedef suggestion *sugg_ptr;
#endif

__SWIRC_BEGIN_DECLS
extern const char g_aff_suffix[];
extern const char g_dic_suffix[];

void spell_init(void);
void spell_deinit(void);

bool spell_word(const char *);
bool spell_wide_word(const wchar_t *);
__SWIRC_END_DECLS

#endif
