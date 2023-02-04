#ifndef SPELL_H
#define SPELL_H

#ifdef __cplusplus
#include <vector>

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

#ifdef __cplusplus
void spell_destroy_suggs(std::vector<sugg_ptr> *);
std::vector<sugg_ptr> *spell_get_suggs(const char *, const wchar_t *);
#endif

void spell_test1(const char *);
void spell_test2(const wchar_t *);

bool spell_word(const char *);
bool spell_wide_word(const wchar_t *);
__SWIRC_END_DECLS

#endif
