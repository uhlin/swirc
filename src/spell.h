#ifndef SPELL_H
#define SPELL_H

#include "readline.h"

#if defined(__cplusplus) && defined(HAVE_HUNSPELL)
#include <vector>

class suggestion {
public:
	suggestion();
	explicit suggestion(CSTRING);
	~suggestion();

	suggestion &operator=(const suggestion &);
	suggestion(const suggestion &);
	suggestion &operator=(suggestion &&);
	suggestion(suggestion &&);

	CSTRING		get_word(void) const;
	CWSTRING	get_wide_word(void) const;

private:
	STRING word;
	WSTRING wide_word;
};

typedef suggestion *sugg_ptr;
#endif

__SWIRC_BEGIN_DECLS
extern bool g_suggs_mode;

#ifdef HAVE_HUNSPELL
void	 spell_init(bool);
void	 spell_deinit(void);

#ifdef __cplusplus
void	 spell_destroy_suggs(std::vector<sugg_ptr> *);
std::vector<sugg_ptr> *
	 spell_get_suggs(CSTRING, CWSTRING);
#endif

void	 spell_test1(CSTRING);
void	 spell_test2(CWSTRING);

bool	 spell_word(CSTRING);
void	 spell_word_readline(volatile struct readline_session_context *);
bool	 spell_wide_word(CWSTRING);
#endif // HAVE_HUNSPELL
__SWIRC_END_DECLS

#endif
