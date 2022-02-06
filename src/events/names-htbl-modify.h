#ifndef NAMES_HTBL_MODIFY_H
#define NAMES_HTBL_MODIFY_H

__SWIRC_BEGIN_DECLS
int	event_names_htbl_modify_owner(const char *, const char *, bool);
int	event_names_htbl_modify_superop(const char *, const char *, bool);
int	event_names_htbl_modify_op(const char *, const char *, bool);
int	event_names_htbl_modify_halfop(const char *, const char *, bool);
int	event_names_htbl_modify_voice(const char *, const char *, bool);
__SWIRC_END_DECLS

#endif
