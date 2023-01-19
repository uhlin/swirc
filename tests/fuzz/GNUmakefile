# Makefile for the append log utility

CC ?= cc
CFLAGS = -O2\
	-Wall\
	-Wsign-compare\
	-Wstrict-prototypes\
	-pipe\
	-std=c11

# C preprocessor flags
CPPFLAGS = -D_BSD_SOURCE=1\
	-D_DEFAULT_SOURCE=1\
	-D_POSIX_C_SOURCE=200809L

RM = rm -f

TGTS = append-log

all: $(TGTS)

append-log: append-log.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	$(RM) $(TGTS)
ifneq ("$(wildcard out)","")
	$(RM) -R out
endif
