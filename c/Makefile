CC=gcc
OUT=build/compiler.out

CFLAGS  =-std=c17 -O1 -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wno-narrowing -Wno-overflow
LDFLAGS =
LIBS=
# LLVM Info
CFLAGS += $(shell llvm-config-14 --cflags)
LDFLAGS +=$(shell llvm-config-14 --ldflags)
LIBS += $(shell llvm-config-14 --libs) $(shell llvm-config-14 --system-libs)
# GLib Info
CFLAGS += $(shell pkg-config glib-2.0 --cflags)
LIBS += $(shell pkg-config glib-2.0 --libs)

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(OUT)