include options.mk

CPPFLAGS += -Isrc/include

PREFIX ?= /usr/local

all: main

include src/commands/build.mk
include src/events/build.mk
include src/build.mk

main: $(TGTS)

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.cpp.o:
	$(E) "  CXX     " $@
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

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
