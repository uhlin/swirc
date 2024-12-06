COMMANDS_DIR = src/commands/
OBJS = $(COMMANDS_DIR)admin.obj\
	$(COMMANDS_DIR)announce.obj\
	$(COMMANDS_DIR)ban.obj\
	$(COMMANDS_DIR)cap.obj\
	$(COMMANDS_DIR)cleartoasts.obj\
	$(COMMANDS_DIR)colormap.obj\
	$(COMMANDS_DIR)connect.obj\
	$(COMMANDS_DIR)ctcp.obj\
	$(COMMANDS_DIR)dcc-w32.obj\
	$(COMMANDS_DIR)dcc.obj\
	$(COMMANDS_DIR)echo.obj\
	$(COMMANDS_DIR)fetchdic.obj\
	$(COMMANDS_DIR)ftp.obj\
	$(COMMANDS_DIR)ignore.obj\
	$(COMMANDS_DIR)info.obj\
	$(COMMANDS_DIR)invite.obj\
	$(COMMANDS_DIR)jp.obj\
	$(COMMANDS_DIR)kick.obj\
	$(COMMANDS_DIR)me.obj\
	$(COMMANDS_DIR)misc.obj\
	$(COMMANDS_DIR)msg.obj\
	$(COMMANDS_DIR)nick.obj\
	$(COMMANDS_DIR)notice.obj\
	$(COMMANDS_DIR)op.obj\
	$(COMMANDS_DIR)sasl-scram-sha.obj\
	$(COMMANDS_DIR)sasl.obj\
	$(COMMANDS_DIR)say.obj\
	$(COMMANDS_DIR)services.obj\
	$(COMMANDS_DIR)servlist.obj\
	$(COMMANDS_DIR)squery.obj\
	$(COMMANDS_DIR)theme.obj\
	$(COMMANDS_DIR)topic.obj\
	$(COMMANDS_DIR)voice.obj\
	$(COMMANDS_DIR)wholeft.obj\
	$(COMMANDS_DIR)znc.obj

CPPFLAGS = $(CPPFLAGS) -I $(COMMANDS_DIR)
