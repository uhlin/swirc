# src/w32.mk

!include ../w32_def.mk

extra_flags=
include_dirs=-Iinclude -Ilibressl-2.4.2-windows/include
library_dirs=-LIBPATH:pdcurses-3.4/x86 -LIBPATH:libressl-2.4.2-windows/x86
log_file=stdout.log

OBJS=assertAPI.obj config.obj curses-funcs.obj cursesInit.obj dataClassify.obj \
     errHand.obj filePred.obj init_once.obj interpreter.obj io-loop.obj        \
     irc.obj libUtils.obj main.obj nestHome.obj net-w32.obj                    \
     network.obj options.obj printtext.obj readline.obj readlineAPI.obj        \
     sig-w32.obj statusbar.obj strHand.obj strcat.obj strcpy.obj               \
     strdup_printf.obj term-w32.obj terminal.obj textBuffer.obj theme.obj      \
     titlebar.obj vcMutex.obj wcscat.obj wcscpy.obj window.obj                 \
     network-openssl.obj

OUT=swirc

.c.obj:
	$(E) ^ ^ CC^ ^ ^ ^ ^ ^ $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c 1>>$(log_file)

$(OUT).exe: fetch_and_expand $(OBJS)
	cd commands && $(MAKE) -f w32.mk
	cd $(MAKEDIR)
	cd events && $(MAKE) -f w32.mk
	cd $(MAKEDIR)
	rc -foswirc.res -v swirc.rc
	$(E) ^ ^ LINK^ ^ ^ ^ $@
	$(Q) $(CC) -Fe$(OUT) *.obj commands/*.obj events/*.obj swirc.res -link $(LDFLAGS) $(library_dirs) $(LDLIBS) 1>>$(log_file)
	$(E) ^ ^ MOVE^ ^ ^ ^ libcrypto-38.dll
	$(Q) move "libressl-2.4.2-windows\x86\libcrypto-38.dll" . 1>>$(log_file)
	$(E) ^ ^ MOVE^ ^ ^ ^ libssl-39.dll
	$(Q) move "libressl-2.4.2-windows\x86\libssl-39.dll" . 1>>$(log_file)
	$(E) ^ ^ MOVE^ ^ ^ ^ pdcurses.dll
	$(Q) move "pdcurses-3.4\x86\pdcurses.dll" . 1>>$(log_file)

fetch_and_expand:
	$(E) ^ ^ FETCH
	$(Q) cscript get_file.js 1>>$(log_file)
	$(E) ^ ^ EXPAND^ ^ libressl-2.4.2-windows.cab
	$(Q) expand libressl-2.4.2-windows.cab "-F:*" . 1>>$(log_file)
	$(E) ^ ^ EXPAND^ ^ pdcurses-3.4.cab
	$(Q) expand pdcurses-3.4.cab "-F:*" . 1>>$(log_file)

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
network-openssl.obj:

clean:
	cd commands && $(MAKE) -f w32.mk clean
	cd $(MAKEDIR)
	cd events && $(MAKE) -f w32.mk clean
	cd $(MAKEDIR)
	$(E) ^ ^ CLEAN
	$(Q) rmdir /s /q libressl-2.3.3-windows
	$(Q) rmdir /s /q pdcurses-3.4
	$(RM) $(OUT).exe $(TEMPFILES) $(log_file)

# EOF
