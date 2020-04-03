include options.mk

CFLAGS+=-Isrc/include
CXXFLAGS+=-Isrc/include

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

RECOMPILE=$(COMMANDS_DIR)ban.o\
	$(COMMANDS_DIR)jp.o\
	$(COMMANDS_DIR)kick.o\
	$(COMMANDS_DIR)op.o\
	$(SRC_DIR)printtext.o

check-init:
	$(RM) $(RECOMPILE)
#	./configure --unittesting
	$(eval CFLAGS += "-DUNIT_TESTING=1")
	$(eval CXXFLAGS += "-DUNIT_TESTING=1")

check: check-init $(OBJS)
	$(Q) strip --strip-symbol=main $(SRC_DIR)main.o
	$(MAKE) -Ctests

# install target
include install.mk

clean:
	$(E) "  CLEAN"
	$(RM) $(OBJS)
	$(RM) $(TGTS)
	$(RM) -R swirc.analyze
	$(RM) swirc.html
	$(RM) swirc.static.html
	$(MAKE) -Ctests clean
