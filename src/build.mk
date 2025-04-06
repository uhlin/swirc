SRC_DIR := src/
TGTS += swirc

OBJS += $(SRC_DIR)assertAPI.o\
	$(SRC_DIR)b64_decode.o\
	$(SRC_DIR)b64_encode.o\
	$(SRC_DIR)config.o\
	$(SRC_DIR)crypt.o\
	$(SRC_DIR)curses-funcs.o\
	$(SRC_DIR)cursesInit.o\
	$(SRC_DIR)dataClassify.o\
	$(SRC_DIR)errHand.o\
	$(SRC_DIR)filePred.o\
	$(SRC_DIR)get_x509_fp.o\
	$(SRC_DIR)icb.o\
	$(SRC_DIR)identd-unix.o\
	$(SRC_DIR)identd.o\
	$(SRC_DIR)initcolors.o\
	$(SRC_DIR)interpreter.o\
	$(SRC_DIR)io-loop.o\
	$(SRC_DIR)irc.o\
	$(SRC_DIR)libUtils.o\
	$(SRC_DIR)log.o\
	$(SRC_DIR)main.o\
	$(SRC_DIR)messagetags.o\
	$(SRC_DIR)nestHome.o\
	$(SRC_DIR)net-unix.o\
	$(SRC_DIR)netsplit.o\
	$(SRC_DIR)network-openssl.o\
	$(SRC_DIR)network.o\
	$(SRC_DIR)nicklist.o\
	$(SRC_DIR)options.o\
	$(SRC_DIR)ossl-scripts.o\
	$(SRC_DIR)printtext.o\
	$(SRC_DIR)pthrMutex.o\
	$(SRC_DIR)readline.o\
	$(SRC_DIR)readlineAPI.o\
	$(SRC_DIR)readlineTabCompletion.o\
	$(SRC_DIR)sig-unix.o\
	$(SRC_DIR)socks.o\
	$(SRC_DIR)spell.o\
	$(SRC_DIR)squeeze_text_deco.o\
	$(SRC_DIR)statusbar.o\
	$(SRC_DIR)strHand.o\
	$(SRC_DIR)strcasestr.o\
	$(SRC_DIR)strcat.o\
	$(SRC_DIR)strcpy.o\
	$(SRC_DIR)strdup_printf.o\
	$(SRC_DIR)strnlen.o\
	$(SRC_DIR)term-unix.o\
	$(SRC_DIR)terminal.o\
	$(SRC_DIR)textBuffer.o\
	$(SRC_DIR)theme.o\
	$(SRC_DIR)titlebar.o\
	$(SRC_DIR)tls-server-unix.o\
	$(SRC_DIR)tls-server.o\
	$(SRC_DIR)wcscat.o\
	$(SRC_DIR)wcscpy.o\
	$(SRC_DIR)window.o\
	$(SRC_DIR)x509_check_host.o

SRCS = $(SRC_DIR)assertAPI.c\
	$(SRC_DIR)b64_decode.c\
	$(SRC_DIR)b64_encode.c\
	$(SRC_DIR)config.cpp\
	$(SRC_DIR)crypt.cpp\
	$(SRC_DIR)curses-funcs.c\
	$(SRC_DIR)cursesInit.c\
	$(SRC_DIR)dataClassify.c\
	$(SRC_DIR)errHand.c\
	$(SRC_DIR)filePred.c\
	$(SRC_DIR)get_x509_fp.cpp\
	$(SRC_DIR)icb.c\
	$(SRC_DIR)identd-unix.cpp\
	$(SRC_DIR)identd.cpp\
	$(SRC_DIR)initcolors.c\
	$(SRC_DIR)interpreter.cpp\
	$(SRC_DIR)io-loop.c\
	$(SRC_DIR)irc.c\
	$(SRC_DIR)libUtils.c\
	$(SRC_DIR)log.c\
	$(SRC_DIR)main.cpp\
	$(SRC_DIR)messagetags.c\
	$(SRC_DIR)nestHome.c\
	$(SRC_DIR)net-unix.c\
	$(SRC_DIR)netsplit.cpp\
	$(SRC_DIR)network-openssl.c\
	$(SRC_DIR)network.cpp\
	$(SRC_DIR)nicklist.cpp\
	$(SRC_DIR)options.c\
	$(SRC_DIR)ossl-scripts.c\
	$(SRC_DIR)printtext.cpp\
	$(SRC_DIR)pthrMutex.c\
	$(SRC_DIR)readline.c\
	$(SRC_DIR)readlineAPI.c\
	$(SRC_DIR)readlineTabCompletion.c\
	$(SRC_DIR)sig-unix.c\
	$(SRC_DIR)socks.cpp\
	$(SRC_DIR)spell.cpp\
	$(SRC_DIR)squeeze_text_deco.cpp\
	$(SRC_DIR)statusbar.cpp\
	$(SRC_DIR)strHand.c\
	$(SRC_DIR)strcasestr.c\
	$(SRC_DIR)strcat.c\
	$(SRC_DIR)strcpy.c\
	$(SRC_DIR)strdup_printf.c\
	$(SRC_DIR)strnlen.c\
	$(SRC_DIR)term-unix.c\
	$(SRC_DIR)terminal.c\
	$(SRC_DIR)textBuffer.c\
	$(SRC_DIR)theme.c\
	$(SRC_DIR)titlebar.c\
	$(SRC_DIR)tls-server-unix.cpp\
	$(SRC_DIR)tls-server.cpp\
	$(SRC_DIR)wcscat.c\
	$(SRC_DIR)wcscpy.c\
	$(SRC_DIR)window.c\
	$(SRC_DIR)x509_check_host.c

$(SRC_DIR)include/swircpaths.h:
	./gen-hdr.sh "$(PREFIX)"

swirc: $(SRC_DIR)include/swircpaths.h $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)
#	$(Q) strip $@
