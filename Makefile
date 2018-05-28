include unix_def.mk

CFLAGS+=-Isrc/include

all: main

include src/commands/build.mk
include src/events/build.mk
include src/build.mk

main: $(TGTS)

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<

# install target
include install.mk

clean:
	$(E) "  CLEAN"
	$(RM) $(OBJS)
	$(RM) $(TGTS)
