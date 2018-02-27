COMMANDS_DIR:=src/commands/
OBJS=$(COMMANDS_DIR)connect.o\
	$(COMMANDS_DIR)jp.o\
	$(COMMANDS_DIR)misc.o\
	$(COMMANDS_DIR)msg.o\
	$(COMMANDS_DIR)say.o\
	$(COMMANDS_DIR)nick.o\
	$(COMMANDS_DIR)topic.o\
	$(COMMANDS_DIR)me.o\
	$(COMMANDS_DIR)kick.o\
	$(COMMANDS_DIR)notice.o\
	$(COMMANDS_DIR)invite.o\
	$(COMMANDS_DIR)services.o\
	$(COMMANDS_DIR)theme.o\
	$(COMMANDS_DIR)sasl.o

CFLAGS+=-I $(COMMANDS_DIR)
