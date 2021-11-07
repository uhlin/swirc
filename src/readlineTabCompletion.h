#ifndef READLINE_TAB_COMPLETION_H
#define READLINE_TAB_COMPLETION_H

__SWIRC_BEGIN_DECLS
PTAB_COMPLETION readline_tab_comp_ctx_new(void);
void readline_tab_comp_ctx_destroy(PTAB_COMPLETION);
void readline_tab_comp_ctx_reset(PTAB_COMPLETION);

void readline_handle_tab(volatile struct readline_session_context *);
__SWIRC_END_DECLS

#endif
