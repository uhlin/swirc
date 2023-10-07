ROOT = ../
COMMANDS_DIR := $(ROOT)src/commands/
EVENTS_DIR := $(ROOT)src/events/
SRC_DIR := $(ROOT)src/

TEST_LOGFILE ?= /tmp/run.log

include $(ROOT)options.mk

CPPFLAGS += -DUNIT_TESTING=1\
	-I$(SRC_DIR)\
	-I$(SRC_DIR)include\
	-I/usr/local/include

LDLIBS += -lcmocka

all: main

include recompile.mk
include tests.mk

main: $(RECOMPILE) $(TESTS)
	./run > "$(TEST_LOGFILE)"
	$(E) "$(TEST_LOGFILE) written"

.SUFFIXES: .c .cpp .o .run

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.cpp.o:
	$(E) "  CXX     " $@
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

.o.run:
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $< \
	    $(COMMANDS_DIR)*.o $(EVENTS_DIR)*.o $(SRC_DIR)*.o \
	    $(LDFLAGS) $(LDLIBS)
	$(Q) strip $@

clean:
	$(E) "  CLEAN"
	$(RM) "$(TEST_LOGFILE)"
	$(RM) $(TESTS)
	$(RM) *.o
