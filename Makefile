include options.mk

CFLAGS += -Isrc/include
CXXFLAGS += -Isrc/include

ifeq ($(shell uname -m),riscv64)
LDFLAGS += -latomic
endif

PREFIX ?= /usr/local

all: main

include src/commands/build.mk
include src/events/build.mk
include src/build.mk

main: $(TGTS)

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(E) "  CXX     " $@
	$(Q) $(CXX) $(CXXFLAGS) -c -o $@ $<

include tests/recompile.mk

check: gen-hdr $(OBJS)
	$(RM) $(RECOMPILE)
	$(Q) strip --strip-symbol=main $(SRC_DIR)main.o
	$(MAKE) -Ctests

# install target
include install.mk

clean:
	$(E) "  CLEAN"
	$(RM) $(COMMANDS_DIR)*.c.smatch
	$(RM) $(EVENTS_DIR)*.c.smatch
	$(RM) $(SRC_DIR)*.c.smatch
	$(RM) $(SRC_DIR)include/swircpaths.h
	$(RM) $(OBJS)
	$(RM) $(TGTS)
	$(RM) -R swirc.analyze
	$(RM) swirc.html
	$(RM) swirc.static.html
	$(MAKE) -Cpo clean
	$(MAKE) -Ctests clean