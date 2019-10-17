#ifndef ICB_H
#define ICB_H

#define ICB_FIELD_SEP "\001"
#define ICB_MESSAGE_MAX 253
#define ICB_PACKET_MAX 255

#ifdef __cplusplus
extern "C" {
#endif

void icb_irc_proxy(char length, char type, const char *pktdata);
void icb_send_group(const char *group);

#ifdef __cplusplus
}
#endif

#endif
