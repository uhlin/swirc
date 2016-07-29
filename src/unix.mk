# src/unix.mk

ROOT=..

include $(ROOT)/unix_def.mk

extra_flags?=-g
include_dirs=-Iinclude
library_dirs=

OBJS=assertAPI.o config.o curses-funcs.o cursesInit.o dataClassify.o
OBJS+=errHand.o filePred.o interpreter.o io-loop.o irc.o
OBJS+=libUtils.o main.o nestHome.o net-unix.o network.o
OBJS+=options.o printtext.o pthrMutex.o readline.o readlineAPI.o
OBJS+=sig-unix.o statusbar.o strHand.o strcat.o strcpy.o
OBJS+=strdup_printf.o term-unix.o terminal.o textBuffer.o theme.o
OBJS+=titlebar.o wcscat.o wcscpy.o window.o network-openssl.o

OUT=swirc

INSTALL=install
PREFIX?=/usr/local
BIN_DIR=$(PREFIX)/bin
MAN_DIR=$(PREFIX)/man/man1
MAN_FILE=swirc.1

.PHONY: clean install

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c

$(OUT): $(OBJS)
	$(Q) cd commands && $(MAKE) -f unix.mk
	$(Q) cd ..
	$(Q) cd events && $(MAKE) -f unix.mk
	$(Q) cd ..
	$(E) "  LINK    " $@
	$(Q) $(CC) $(library_dirs) $(LDFLAGS) -o $(OUT) *.o commands/*.o events/*.o $(LDLIBS)

assertAPI.o:
config.o:
curses-funcs.o:
cursesInit.o:
dataClassify.o:
errHand.o:
filePred.o:
interpreter.o:
io-loop.o:
irc.o:
libUtils.o:
main.o:
nestHome.o:
net-unix.o:
network.o:
options.o:
printtext.o:
pthrMutex.o:
readline.o:
readlineAPI.o:
sig-unix.o:
statusbar.o:
strHand.o:
strcat.o:
strcpy.o:
strdup_printf.o:
term-unix.o:
terminal.o:
textBuffer.o:
theme.o:
titlebar.o:
wcscat.o:
wcscpy.o:
window.o:
network-openssl.o:

clean:
	$(Q) cd commands && $(MAKE) -f unix.mk clean
	$(Q) cd ..
	$(Q) cd events && $(MAKE) -f unix.mk clean
	$(Q) cd ..
	$(E) "  CLEAN"
	$(RM) $(OUT) $(TEMPFILES)

install: $(OUT) $(MAN_FILE)
	$(INSTALL) -d $(BIN_DIR)
	$(INSTALL) -d $(MAN_DIR)
	$(INSTALL) -m 0755 $(OUT) $(BIN_DIR)/$(OUT)
	$(INSTALL) -m 0444 $(MAN_FILE) $(MAN_DIR)/$(MAN_FILE)

# EOF
