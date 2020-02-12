SRC_DIR:=src/
TGTS+=swirc

OBJS+=$(SRC_DIR)assertAPI.o\
	$(SRC_DIR)b64_decode.o\
	$(SRC_DIR)b64_encode.o\
	$(SRC_DIR)config.o\
	$(SRC_DIR)curses-funcs.o\
	$(SRC_DIR)cursesInit.o\
	$(SRC_DIR)dataClassify.o\
	$(SRC_DIR)errHand.o\
	$(SRC_DIR)filePred.o\
	$(SRC_DIR)icb.o\
	$(SRC_DIR)interpreter.o\
	$(SRC_DIR)io-loop.o\
	$(SRC_DIR)irc.o\
	$(SRC_DIR)libUtils.o\
	$(SRC_DIR)log.o\
	$(SRC_DIR)main.o\
	$(SRC_DIR)nestHome.o\
	$(SRC_DIR)net-unix.o\
	$(SRC_DIR)network-openssl.o\
	$(SRC_DIR)network.o\
	$(SRC_DIR)options.o\
	$(SRC_DIR)printtext.o\
	$(SRC_DIR)pthrMutex.o\
	$(SRC_DIR)readline.o\
	$(SRC_DIR)readlineAPI.o\
	$(SRC_DIR)readlineTabCompletion.o\
	$(SRC_DIR)sig-unix.o\
	$(SRC_DIR)statusbar.o\
	$(SRC_DIR)strHand.o\
	$(SRC_DIR)strcat.o\
	$(SRC_DIR)strcpy.o\
	$(SRC_DIR)strdup_printf.o\
	$(SRC_DIR)term-unix.o\
	$(SRC_DIR)terminal.o\
	$(SRC_DIR)textBuffer.o\
	$(SRC_DIR)theme.o\
	$(SRC_DIR)titlebar.o\
	$(SRC_DIR)wcscat.o\
	$(SRC_DIR)wcscpy.o\
	$(SRC_DIR)window.o\
	$(SRC_DIR)x509_check_host.o

swirc: $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)
	$(Q) strip $@
