COMMANDS_DIR := src/commands/
OBJS = $(COMMANDS_DIR)admin.o\
	$(COMMANDS_DIR)announce.o\
	$(COMMANDS_DIR)ban.o\
	$(COMMANDS_DIR)cap.o\
	$(COMMANDS_DIR)cleartoasts.o\
	$(COMMANDS_DIR)colormap.o\
	$(COMMANDS_DIR)connect.o\
	$(COMMANDS_DIR)ctcp.o\
	$(COMMANDS_DIR)dcc-unix.o\
	$(COMMANDS_DIR)dcc.o\
	$(COMMANDS_DIR)echo.o\
	$(COMMANDS_DIR)fetchdic.o\
	$(COMMANDS_DIR)ftp-unix.o\
	$(COMMANDS_DIR)ftp.o\
	$(COMMANDS_DIR)ignore.o\
	$(COMMANDS_DIR)info.o\
	$(COMMANDS_DIR)invite.o\
	$(COMMANDS_DIR)jp.o\
	$(COMMANDS_DIR)kick.o\
	$(COMMANDS_DIR)me.o\
	$(COMMANDS_DIR)misc.o\
	$(COMMANDS_DIR)msg.o\
	$(COMMANDS_DIR)nick.o\
	$(COMMANDS_DIR)notice.o\
	$(COMMANDS_DIR)op.o\
	$(COMMANDS_DIR)sasl-scram-sha.o\
	$(COMMANDS_DIR)sasl.o\
	$(COMMANDS_DIR)say.o\
	$(COMMANDS_DIR)services.o\
	$(COMMANDS_DIR)servlist.o\
	$(COMMANDS_DIR)squery.o\
	$(COMMANDS_DIR)theme.o\
	$(COMMANDS_DIR)topic.o\
	$(COMMANDS_DIR)voice.o\
	$(COMMANDS_DIR)wholeft.o\
	$(COMMANDS_DIR)znc.o

CPPFLAGS += -I $(COMMANDS_DIR)
