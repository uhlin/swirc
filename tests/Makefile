# tests/Makefile

ROOT=..
SRC=$(ROOT)/src

include $(ROOT)/unix_def.mk

# CFLAGS+=
# LDFLAGS+=
LDLIBS+=-lcmocka

extra_flags?=
include_dirs=-I$(SRC) -I$(SRC)/include -I/usr/local/include
library_dirs=-L/usr/local/lib

TESTS=test_strdup_printf.run
TESTS+=test_printtext.run

.PHONY: all objects clean clean_all
.SUFFIXES: .c .o .run

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(include_dirs) $(CFLAGS) $(extra_flags) -c $*.c

.o.run:
	$(E) "  LINK    " $@
	$(Q) $(CC) $(library_dirs) $(LDFLAGS) -o $*.run $*.o \
	$(SRC)/*.o $(SRC)/commands/*.o $(SRC)/events/*.o $(LDLIBS)
	$(E) "  STRIP   " $@
	$(Q) strip $@

all: objects $(TESTS)
	./run

objects:
	$(Q) extra_flags="-DUNIT_TESTING=1" \
	$(MAKE) -C$(SRC) -f unix.mk objects
	$(Q) extra_flags="" $(MAKE) -C$(SRC)/commands -f unix.mk all
	$(Q) extra_flags="" $(MAKE) -C$(SRC)/events -f unix.mk all

test_printtext.run: test_printtext.o
test_strdup_printf.run: test_strdup_printf.o

test_printtext.o:
test_strdup_printf.o:

clean:
	$(E) "  CLEAN"
	$(RM) $(TEMPFILES) $(TESTS)

clean_all:
	$(Q) $(MAKE) -C$(SRC) -f unix.mk clean
	$(E) "  CLEAN"
	$(RM) $(TEMPFILES) $(TESTS)
