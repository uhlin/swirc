#ifndef READLINE_TAB_COMPLETION_H
#define READLINE_TAB_COMPLETION_H

#define servhost_macro(x)\
	(#x " services."),\
	(#x " services.alphachat.net"),\
	(#x " services.anonops.com"),\
	(#x " services.irc.ircnow.org"),\
	(#x " services.oftc.net")

typedef void (*AC_FUNC)(volatile struct readline_session_context *, CSTRING);

__SWIRC_BEGIN_DECLS
PTAB_COMPLETION readline_tab_comp_ctx_new(void);
void readline_tab_comp_ctx_destroy(PTAB_COMPLETION);
void readline_tab_comp_ctx_reset(PTAB_COMPLETION);

void readline_handle_tab(volatile struct readline_session_context *);
__SWIRC_END_DECLS

#endif
