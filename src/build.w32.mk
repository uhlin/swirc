SRC_DIR=src/
TGTS=$(TGTS) swirc.exe

OBJS=$(OBJS)\
	$(SRC_DIR)DesktopNotificationManagerCompat.obj\
	$(SRC_DIR)ToastsAPI.obj\
	$(SRC_DIR)assertAPI.obj\
	$(SRC_DIR)b64_decode.obj\
	$(SRC_DIR)b64_encode.obj\
	$(SRC_DIR)config.obj\
	$(SRC_DIR)curses-funcs.obj\
	$(SRC_DIR)cursesInit.obj\
	$(SRC_DIR)dataClassify.obj\
	$(SRC_DIR)errHand.obj\
	$(SRC_DIR)filePred.obj\
	$(SRC_DIR)init_once.obj\
	$(SRC_DIR)interpreter.obj\
	$(SRC_DIR)io-loop.obj\
	$(SRC_DIR)irc.obj\
	$(SRC_DIR)libUtils.obj\
	$(SRC_DIR)main.obj\
	$(SRC_DIR)nestHome.obj\
	$(SRC_DIR)net-w32.obj\
	$(SRC_DIR)network-openssl.obj\
	$(SRC_DIR)network.obj\
	$(SRC_DIR)options.obj\
	$(SRC_DIR)printtext.obj\
	$(SRC_DIR)readline.obj\
	$(SRC_DIR)readlineAPI.obj\
	$(SRC_DIR)sig-w32.obj\
	$(SRC_DIR)statusbar.obj\
	$(SRC_DIR)strHand.obj\
	$(SRC_DIR)strcat.obj\
	$(SRC_DIR)strcpy.obj\
	$(SRC_DIR)strdup_printf.obj\
	$(SRC_DIR)term-w32.obj\
	$(SRC_DIR)terminal.obj\
	$(SRC_DIR)textBuffer.obj\
	$(SRC_DIR)theme.obj\
	$(SRC_DIR)titlebar.obj\
	$(SRC_DIR)vcMutex.obj\
	$(SRC_DIR)wcscat.obj\
	$(SRC_DIR)wcscpy.obj\
	$(SRC_DIR)window.obj\
	$(SRC_DIR)x509_check_host.obj

swirc.exe: fetch_and_expand $(OBJS)
	rc -foswirc.res -v $(SRC_DIR)swirc.rc
	$(CC) -Feswirc $(OBJS) swirc.res -link $(LDFLAGS) $(LDLIBS)

fetch_and_expand:
	cscript $(SRC_DIR)get_file.js
	expand curl-$(CURL_VERSION).cab "-F:*" .
	expand libressl-$(LIBRESSL_VERSION)-windows.cab "-F:*" .
	expand pdcurses-$(PDCURSES_VERSION).cab "-F:*" .
