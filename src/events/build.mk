EVENTS_DIR := src/events/
OBJS += $(EVENTS_DIR)account.o\
	$(EVENTS_DIR)auth.o\
	$(EVENTS_DIR)away.o\
	$(EVENTS_DIR)banlist.o\
	$(EVENTS_DIR)batch.o\
	$(EVENTS_DIR)cap.o\
	$(EVENTS_DIR)channel.o\
	$(EVENTS_DIR)chghost.o\
	$(EVENTS_DIR)error.o\
	$(EVENTS_DIR)invite.o\
	$(EVENTS_DIR)list.o\
	$(EVENTS_DIR)misc.o\
	$(EVENTS_DIR)motd.o\
	$(EVENTS_DIR)names-htbl-modify.o\
	$(EVENTS_DIR)names.o\
	$(EVENTS_DIR)noop.o\
	$(EVENTS_DIR)notice.o\
	$(EVENTS_DIR)ping.o\
	$(EVENTS_DIR)pong.o\
	$(EVENTS_DIR)privmsg.o\
	$(EVENTS_DIR)servlist.o\
	$(EVENTS_DIR)stats.o\
	$(EVENTS_DIR)wallops.o\
	$(EVENTS_DIR)welcome-unix.o\
	$(EVENTS_DIR)welcome.o\
	$(EVENTS_DIR)whois.o

CPPFLAGS += -I $(EVENTS_DIR)
