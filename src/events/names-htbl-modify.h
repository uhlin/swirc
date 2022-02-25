#ifndef NAMES_HTBL_MODIFY_H
#define NAMES_HTBL_MODIFY_H

#ifdef __cplusplus
namespace names_htbl_modify {
	int owner(const char *, const char *, bool);
	int superop(const char *, const char *, bool);
	int op(const char *, const char *, bool);
	int halfop(const char *, const char *, bool);
	int voice(const char *, const char *, bool);
}
#endif

#endif
