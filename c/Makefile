CC=x86_64-w64-mingw32-gcc
OUT=out/compiler.exe

CFLAGS  =-std=c17 -O3 -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wno-narrowing -Wno-overflow
LDFLAGS =-lmingw32

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.obj

mrproper: clean
	rm -rf $(OUT)