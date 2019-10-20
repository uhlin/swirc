#ifndef ICB_H
#define ICB_H

#define ICB_FIELD_SEP "\001"
#define ICB_MESSAGE_MAX 253
#define ICB_PACKET_MAX 255

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool g_icb_processing_names;

void icb_irc_proxy(const int length, const char type, const char *pktdata);
void icb_process_event_eof_names(void);
void icb_send_group(const char *group);
void icb_send_users(const char *arg);

#ifdef __cplusplus
}
#endif

#endif
