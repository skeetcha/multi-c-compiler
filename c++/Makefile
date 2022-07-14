CC=x86_64-w64-mingw32-g++
OUT=out/compiler.exe

CFLAGS  =-std=c++17 -O3 -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wno-narrowing -Wno-overflow
LDFLAGS =-lmingw32

SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.obj)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.obj: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.obj

mrproper: clean
	rm -rf $(OUT)