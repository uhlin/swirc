EVENTS_DIR := src/events/
EVENTS_SRCS = $(EVENTS_DIR)account.cpp\
	$(EVENTS_DIR)auth.c\
	$(EVENTS_DIR)away.cpp\
	$(EVENTS_DIR)banlist.cpp\
	$(EVENTS_DIR)batch.cpp\
	$(EVENTS_DIR)cap.c\
	$(EVENTS_DIR)channel.cpp\
	$(EVENTS_DIR)chghost.cpp\
	$(EVENTS_DIR)error.c\
	$(EVENTS_DIR)invite.cpp\
	$(EVENTS_DIR)list.cpp\
	$(EVENTS_DIR)misc.cpp\
	$(EVENTS_DIR)motd.c\
	$(EVENTS_DIR)names-htbl-modify.cpp\
	$(EVENTS_DIR)names.cpp\
	$(EVENTS_DIR)noop.c\
	$(EVENTS_DIR)notice.cpp\
	$(EVENTS_DIR)ping.c\
	$(EVENTS_DIR)pong.c\
	$(EVENTS_DIR)privmsg.cpp\
	$(EVENTS_DIR)servlist.cpp\
	$(EVENTS_DIR)stats.cpp\
	$(EVENTS_DIR)wallops.cpp\
	$(EVENTS_DIR)welcome-unix.c\
	$(EVENTS_DIR)welcome.cpp\
	$(EVENTS_DIR)whois.cpp

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
