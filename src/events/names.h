#ifndef NAMES_H
#define NAMES_H

int	event_names_htbl_insert        (const char *nick, const char *channel);
#if 0
int	event_names_htbl_modify        (const char *nick, const char *channel, bool is_op, bool is_halfop, bool is_voice);
#endif
int	event_names_htbl_modify_op     (const char *nick, const char *channel, bool is_op);
int	event_names_htbl_modify_halfop (const char *nick, const char *channel, bool is_halfop);
int	event_names_htbl_modify_voice  (const char *nick, const char *channel, bool is_voice);
int	event_names_htbl_remove        (const char *nick, const char *channel);
int	event_names_print_all          (const char *channel);
void	event_eof_names                (struct irc_message_compo *);
void	event_names                    (struct irc_message_compo *);
void	event_names_deinit             (void);
void	event_names_htbl_remove_all    (PIRC_WINDOW);
void	event_names_init               (void);

#endif
