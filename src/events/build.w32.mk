EVENTS_DIR=src/events/
OBJS=$(OBJS)\
	$(EVENTS_DIR)account.obj\
	$(EVENTS_DIR)auth.obj\
	$(EVENTS_DIR)away.obj\
	$(EVENTS_DIR)banlist.obj\
	$(EVENTS_DIR)cap.obj\
	$(EVENTS_DIR)channel.obj\
	$(EVENTS_DIR)error.obj\
	$(EVENTS_DIR)invite.obj\
	$(EVENTS_DIR)list.obj\
	$(EVENTS_DIR)misc.obj\
	$(EVENTS_DIR)motd.obj\
	$(EVENTS_DIR)names-htbl-modify.obj\
	$(EVENTS_DIR)names.obj\
	$(EVENTS_DIR)noop.obj\
	$(EVENTS_DIR)notice.obj\
	$(EVENTS_DIR)ping.obj\
	$(EVENTS_DIR)pong.obj\
	$(EVENTS_DIR)privmsg.obj\
	$(EVENTS_DIR)servlist.obj\
	$(EVENTS_DIR)wallops.obj\
	$(EVENTS_DIR)welcome-w32.obj\
	$(EVENTS_DIR)welcome.obj\
	$(EVENTS_DIR)whois.obj

CFLAGS=$(CFLAGS) -I $(EVENTS_DIR)
