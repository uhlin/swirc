# src/unix.mk

ROOT=..

include $(ROOT)/unix_def.mk

ifeq ($(CC), gcc)
include_dirs=-iquoteinclude
else
include_dirs=-Iinclude
endif

library_dirs=
extra_flags=

OBJS=assertAPI.o config.o curses-funcs.o cursesInit.o dataClassify.o
OBJS+=errHand.o filePred.o interpreter.o io-loop.o irc.o
OBJS+=libUtils.o main.o nestHome.o net-unix.o network.o
OBJS+=options.o printtext.o pthrMutex.o readline.o readlineAPI.o
OBJS+=sig-unix.o statusbar.o strHand.o strcat.o strcpy.o
OBJS+=strdup_printf.o term-unix.o terminal.o textBuffer.o theme.o
OBJS+=titlebar.o wcscat.o wcscpy.o window.o

OUT=swirc

.PHONY: clean

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c

$(OUT): $(OBJS)
	$(Q) cd events && $(MAKE) -f unix.mk
	$(Q) cd ..
	$(E) "  LINK    " $@
	$(Q) $(CC) $(library_dirs) $(LDFLAGS) -o $(OUT) *.o events/*.o $(LDLIBS)

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

clean:
	$(Q) cd events && $(MAKE) -f unix.mk clean
	$(Q) cd ..
	$(E) "  CLEAN"
	$(RM) $(OUT) $(TEMPFILES)

# EOF
