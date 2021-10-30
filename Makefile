# Makefile
#
LIB = xpr
SRC = $(wildcard *.c)
HDR = $(wildcard *.h)

OBJ = $(SRC:%.c=%.o)
PICOBJ = $(OBJ:%.o=%-pic.o)
THELIB = $(LIB:%=lib%.so)
BIN = $(SRC:%.c=%)

CC    ?= cc
AFLCC ?= afl-clang
LD    := $(CC)
AFLLD := $(AFLCC)
RM    ?= rm
SHELL ?= sh

CFLAGS = -std=c99 -D_XOPEN_SOURCE=700
PICFLAGS = -fPIC
LDFLAGS =
LDLIBS  = -lm
OPT = -O3 -march=native -mtune=native
WARN = -Wall -Wextra

CFLAGS += $(OPT) $(WARN)

ifeq "${VERBOSE}" ""
E=@printf "%-4s %s\n"
Q=@
else
E=@printf "`tput setf 4`%-4s %s`tput sgr0`\n"
Q=
endif

.PHONY: all
all: $(THELIB) $(BIN)

$(THELIB): $(PICOBJ)
	$E "LD.L" "$@"
	$Q $(LD) $(LDFLAGS) -shared -o $@ $^ $(LDLIBS)

%-pic.o: %.c $(HDR)
	$E "CC.L" "$<"
	$Q $(CC) $(CFLAGS) $(PICFLAGS) -c -o $@ $<

%.o: %.c
	$E "CC.X" "$^"
	$Q $(CC) $(CFLAGS) -DMAIN -c -o $@ $<

$(BIN): %: %.o
	$E "LD.X" "$@"
	$Q $(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean
clean:
	$E "CLEAN" ""
	$Q $(RM) $(OBJ) $(PICOBJ) $(THELIB) $(BIN)

.PHONY: ci
ci: xpr tst
	$Q ./tst <test.in

fuzzme.o: CC=$(AFLCC)
fuzzme: LD=$(AFLLD)
