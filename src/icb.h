#ifndef ICB_H
#define ICB_H

#define ICB_FIELD_SEP		"\001"
#define ICB_MESSAGE_MAX		253
#define ICB_PACKET_MAX		255

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool g_icb_processing_names;

void	 icb_irc_proxy(const int length, const char type, const char *pktdata);
void	 icb_process_event_eof_names(void);
void	 icb_send_beep(const char *arg);
void	 icb_send_boot(const char *victim);
void	 icb_send_group(const char *group);
void	 icb_send_name(const char *new_nick);
void	 icb_send_noop(void);
void	 icb_send_open_msg(const char *text);
void	 icb_send_ping(const char *arg);
void	 icb_send_pm(const char *to_who, const char *text);
void	 icb_send_pong(const char *arg);
void	 icb_send_topic(const char *new_topic);
void	 icb_send_users(const char *arg);

#ifdef __cplusplus
}
#endif

#endif
