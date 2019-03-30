INSTALL=install
INSTALL_DEPS=swirc src/swirc.1 swirc.conf.5 swirc-royal.png
PREFIX?=/usr/local

# Don't provide a default value for DESTDIR. It should be empty.
DESTDIR?=

DEST_PROGRAM=$(DESTDIR)$(PREFIX)/bin
DEST_MANUAL=$(DESTDIR)$(PREFIX)/man/man1
DEST_CONFMAN=$(DESTDIR)$(PREFIX)/man/man5
DEST_LOGO=$(DESTDIR)$(PREFIX)/share/swirc

install: $(INSTALL_DEPS)
	$(INSTALL) -d $(DEST_PROGRAM)
	$(INSTALL) -d $(DEST_MANUAL)
	$(INSTALL) -d $(DEST_CONFMAN)
	$(INSTALL) -d $(DEST_LOGO)
	$(INSTALL) -m 0755 swirc $(DEST_PROGRAM)
	$(INSTALL) -m 0444 src/swirc.1 $(DEST_MANUAL)
	$(INSTALL) -m 0444 swirc.conf.5 $(DEST_CONFMAN)
	$(INSTALL) -m 0444 swirc-royal.png $(DEST_LOGO)
