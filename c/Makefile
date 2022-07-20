CC=gcc
OUT=build/compiler.out

CFLAGS  =-std=c17 -O3 -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wno-narrowing -Wno-overflow
LDFLAGS =
LIBS=
# LLVM Info
CFLAGS += $(shell llvm-config-14 --cxxflags)
LDFLAGS +=$(shell llvm-config-14 --ldflags)
LIBS += $(shell llvm-config-14 --libs) $(shell llvm-config-14 --system-libs)

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