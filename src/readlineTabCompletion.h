#ifndef READLINE_TAB_COMPLETION_H
#define READLINE_TAB_COMPLETION_H

#ifdef __cplusplus
extern "C" {
#endif

PTAB_COMPLETION readline_tab_comp_ctx_new(void);
void readline_tab_comp_ctx_destroy(PTAB_COMPLETION ctx);
void readline_tab_comp_ctx_reset(PTAB_COMPLETION ctx);

void readline_handle_tab(volatile struct readline_session_context *);

#ifdef __cplusplus
}
#endif

#endif
