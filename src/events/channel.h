#ifndef CHANNEL_H
#define CHANNEL_H

void event_topic         (struct irc_message_compo *);
void event_topic_creator (struct irc_message_compo *);
void event_mode          (struct irc_message_compo *);
void event_join          (struct irc_message_compo *);

#endif
