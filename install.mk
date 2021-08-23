INSTALL := install
INSTALL_DEPS = po/de/swirc.mo\
	po/fi/swirc.mo\
	po/fr/swirc.mo\
	po/sv/swirc.mo\
	src/swirc.1\
	swirc\
	swirc-royal.png\
	swirc.conf.5

PREFIX ?= /usr/local

# Don't provide a default value for DESTDIR. It should be empty.
DESTDIR ?=

DEST_PROGRAM	= $(DESTDIR)$(PREFIX)/bin
DEST_MANUAL	= $(DESTDIR)$(PREFIX)/share/man/man1
DEST_CONFMAN	= $(DESTDIR)$(PREFIX)/share/man/man5
DEST_LOGO	= $(DESTDIR)$(PREFIX)/share/swirc
DEST_LC_MSGS	= $(DESTDIR)$(PREFIX)/share/locale/

install: $(INSTALL_DEPS)
	$(INSTALL) -d $(DEST_PROGRAM)
	$(INSTALL) -d $(DEST_MANUAL)
	$(INSTALL) -d $(DEST_CONFMAN)
	$(INSTALL) -d $(DEST_LOGO)
	$(INSTALL) -d $(DEST_LC_MSGS)de/LC_MESSAGES
	$(INSTALL) -d $(DEST_LC_MSGS)fi/LC_MESSAGES
	$(INSTALL) -d $(DEST_LC_MSGS)fr/LC_MESSAGES
	$(INSTALL) -d $(DEST_LC_MSGS)sv/LC_MESSAGES
	$(INSTALL) -m 0755 swirc $(DEST_PROGRAM)
	$(INSTALL) -m 0444 src/swirc.1 $(DEST_MANUAL)
	$(INSTALL) -m 0444 swirc.conf.5 $(DEST_CONFMAN)
	$(INSTALL) -m 0444 swirc-royal.png $(DEST_LOGO)
	$(INSTALL) -m 0644 po/de/swirc.mo $(DEST_LC_MSGS)de/LC_MESSAGES
	$(INSTALL) -m 0644 po/fi/swirc.mo $(DEST_LC_MSGS)fi/LC_MESSAGES
	$(INSTALL) -m 0644 po/fr/swirc.mo $(DEST_LC_MSGS)fr/LC_MESSAGES
	$(INSTALL) -m 0644 po/sv/swirc.mo $(DEST_LC_MSGS)sv/LC_MESSAGES
