ROOT=..
COMMANDS_DIR:=$(ROOT)/src/commands/
EVENTS_DIR:=$(ROOT)/src/events/
SRC_DIR:=$(ROOT)/src/

include $(ROOT)/options.mk

CFLAGS+=-DUNIT_TESTING=1\
	-I$(SRC_DIR)\
	-I$(SRC_DIR)include\
	-I/usr/local/include

LDLIBS+=-lcmocka

all: main

include recompile.mk
include tests.mk

main: $(RECOMPILE) $(TESTS)

.SUFFIXES: .c .o .run

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<

.o.run:
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $< \
	    $(COMMANDS_DIR)*.o $(EVENTS_DIR)*.o $(SRC_DIR)*.o \
	    $(LDFLAGS) $(LDLIBS)
	$(Q) strip $@

clean:
	$(E) "  CLEAN"
	$(RM) $(TESTS)
	$(RM) *.o
