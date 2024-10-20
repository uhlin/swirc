#ifndef SRC_MESSAGETAGS_H_
#define SRC_MESSAGETAGS_H_

struct messagetags {
	STRING	account;
	STRING	batch;
	STRING	srv_time;
};

__SWIRC_BEGIN_DECLS
struct messagetags *
	msgtags_get(CSTRING);
void	msgtags_free(struct messagetags *);

void	msgtags_handle_batch(CSTRING, const struct messagetags *) NONNULL;
void	msgtags_process(struct irc_message_compo *, struct messagetags *)
	    NONNULL;
__SWIRC_END_DECLS

#endif
