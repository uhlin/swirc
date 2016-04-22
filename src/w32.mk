# src/w32.mk

!include ../w32_def.mk

extra_flags=
include_dirs=-Iinclude
library_dirs=-LIBPATH:pdcurses-3.4
log_file=stdout.log

OBJS=assertAPI.obj config.obj curses-funcs.obj cursesInit.obj dataClassify.obj \
     errHand.obj filePred.obj init_once.obj interpreter.obj io-loop.obj        \
     irc.obj libUtils.obj main.obj nestHome.obj net-w32.obj                    \
     network.obj options.obj printtext.obj readline.obj readlineAPI.obj        \
     sig-w32.obj statusbar.obj strHand.obj strcat.obj strcpy.obj               \
     strdup_printf.obj term-w32.obj terminal.obj textBuffer.obj theme.obj      \
     titlebar.obj vcMutex.obj wcscat.obj wcscpy.obj window.obj

OUT=swirc

.c.obj:
	$(E) ^ ^ CC^ ^ ^ ^ ^ ^ $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c 1>>$(log_file)

$(OUT).exe: $(OBJS)
	cd commands && $(MAKE) -f w32.mk
	cd $(MAKEDIR)
	cd events && $(MAKE) -f w32.mk
	cd $(MAKEDIR)
	$(E) ^ ^ FETCH^ ^ ^ pdcurses-3.4.cab
	$(Q) cscript get_file.js 1>>$(log_file)
	$(E) ^ ^ MKDIR^ ^ ^ pdcurses-3.4
	$(Q) mkdir pdcurses-3.4 1>>$(log_file)
	$(E) ^ ^ EXPAND^ ^ pdcurses-3.4.cab
	$(Q) expand pdcurses-3.4.cab "-F:*" pdcurses-3.4 1>>$(log_file)
	$(E) ^ ^ LINK^ ^ ^ ^ $@
	$(Q) $(CC) -Fe$(OUT) *.obj commands/*.obj events/*.obj -link $(LDFLAGS) $(library_dirs) $(LDLIBS) 1>>$(log_file)
	$(E) ^ ^ MOVE^ ^ ^ ^ pdcurses.dll
	$(Q) move "pdcurses-3.4\pdcurses.dll" . 1>>$(log_file)

assertAPI.obj:
config.obj:
curses-funcs.obj:
cursesInit.obj:
dataClassify.obj:
errHand.obj:
filePred.obj:
init_once.obj:
interpreter.obj:
io-loop.obj:
irc.obj:
libUtils.obj:
main.obj:
nestHome.obj:
net-w32.obj:
network.obj:
options.obj:
printtext.obj:
readline.obj:
readlineAPI.obj:
sig-w32.obj:
statusbar.obj:
strHand.obj:
strcat.obj:
strcpy.obj:
strdup_printf.obj:
term-w32.obj:
terminal.obj:
textBuffer.obj:
theme.obj:
titlebar.obj:
vcMutex.obj:
wcscat.obj:
wcscpy.obj:
window.obj:

clean:
	cd commands && $(MAKE) -f w32.mk clean
	cd $(MAKEDIR)
	cd events && $(MAKE) -f w32.mk clean
	cd $(MAKEDIR)
	$(E) ^ ^ CLEAN
	$(Q) rmdir /s /q pdcurses-3.4
	$(RM) $(OUT).exe $(TEMPFILES) $(log_file)

# EOF
