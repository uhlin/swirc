#ifndef ICB_H
#define ICB_H

#define ICB_FIELD_SEP "\001"

#ifdef __cplusplus
extern "C" {
#endif

void icb_irc_proxy(char length, char type, const char *pktdata);

#ifdef __cplusplus
}
#endif

#endif
