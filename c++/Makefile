CC=g++
OUT=build/compiler.out

CFLAGS  =-std=c++17 -O3 -Wall -Wextra -Wpedantic -Wstrict-aliasing
LDFLAGS =
LIBS=
# LLVM Info
CFLAGS +=$(shell llvm-config-14 --cxxflags)
LDFLAGS +=$(shell llvm-config-14 --ldflags)
LIBS +=$(shell llvm-config-14 --libs) $(shell llvm-config-14 --system-libs)

SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.obj)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

%.obj: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.obj

mrproper: clean
	rm -rf $(OUT)