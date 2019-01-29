ROOT=..
SRC=$(ROOT)/src

include $(ROOT)/unix_def.mk

# CFLAGS+=-DUNIT_TESTING=1
CFLAGS+=-I$(SRC) -I$(SRC)/include -I/usr/local/include
# LDFLAGS+=-lcmocka

TESTS=\
	int_diff.run\
	int_sum.run\
	is_alphabetic.run\
	is_numeric.run\
	realloc_strcat.run\
	size_product.run\
	squeeze.run\
	squeeze_text_deco.run\
	strColor.run\
	strFeed.run\
	strToLower.run\
	strToUpper.run\
	strdup_printf.run\
	sw_snprintf.run\
	sw_strcat.run\
	sw_strcpy.run\
	sw_strdup.run\
	sw_wcscat.run\
	sw_wcscpy.run\
	trim.run\
	write_to_stream.run

.SUFFIXES: .c .o .run

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<

.o.run:
	$(Q) strip --strip-symbol=main $(SRC)/main.o
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $< $(SRC)/*.o $(SRC)/commands/*.o $(SRC)/events/*.o $(LDFLAGS) $(LDLIBS)
	$(Q) strip $@

all: progobjs $(TESTS)

progobjs:
	$(RM) $(SRC)/main.o
	$(MAKE) -C$(ROOT)

clean:
	$(MAKE) -C$(ROOT) clean
	$(RM) $(TESTS)
	$(RM) *.o
