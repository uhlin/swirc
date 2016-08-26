# src/commands/unix.mk

ROOT=../..

include $(ROOT)/unix_def.mk

extra_flags?=-g
include_dirs=-I../include
library_dirs=

OBJS=connect.o jp.o misc.o msg.o say.o nick.o topic.o me.o
OBJS+=kick.o notice.o invite.o services.o

.PHONY: all clean

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c

all: $(OBJS)

connect.o:
jp.o:
misc.o:
msg.o:
say.o:
nick.o:
topic.o:
me.o:
kick.o:
notice.o:
invite.o:
services.o:

clean:
	$(E) "  CLEAN"
	$(RM) $(TEMPFILES)

# EOF
