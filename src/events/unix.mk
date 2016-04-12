# src/events/unix.mk

ROOT=../..

include $(ROOT)/unix_def.mk

extra_flags=
include_dirs=-I../include
library_dirs=

OBJS=channel.o error.o misc.o motd.o names.o
OBJS+=noop.o notice.o ping.o welcome-unix.o welcome.o
OBJS+=whois.o

.PHONY: all clean

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c

all: $(OBJS)

channel.o:
error.o:
misc.o:
motd.o:
names.o:
noop.o:
notice.o:
ping.o:
welcome-unix.o:
welcome.o:
whois.o:

clean:
	$(E) "  CLEAN"
	$(RM) $(TEMPFILES)

# EOF
