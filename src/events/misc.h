#ifndef MISC_H
#define MISC_H

#ifdef __cplusplus
extern "C" {
#endif

void event_allaround_extract_find_colon   (struct irc_message_compo *);
void event_allaround_extract_remove_colon (struct irc_message_compo *);
void event_serverFeatures                 (struct irc_message_compo *);
void event_channelCreatedWhen             (struct irc_message_compo *);
void event_channelModeIs                  (struct irc_message_compo *);
void event_channel_forward                (struct irc_message_compo *);
void event_local_and_global_users         (struct irc_message_compo *);
void event_nicknameInUse                  (struct irc_message_compo *);
void event_userModeIs                     (struct irc_message_compo *);
void event_youAreOper                     (struct irc_message_compo *);

#ifdef __cplusplus
}
#endif

#endif
