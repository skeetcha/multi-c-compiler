#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdbool.h>
#include "token.h"

typedef struct {
    int line;
    char putback;
    FILE* inFile;
} Compiler;

Compiler Compiler_new();
bool Compiler_scan(Compiler* self, Token* t);
char Compiler_skip(Compiler* self);
int Compiler_scanint(Compiler* self, char c);
char Compiler_next(Compiler* self);
void Compiler_putback(Compiler* self, char c);
void Compiler_scanfile(Compiler* self);
void Compiler_run(Compiler* self, int argc, char* argv[]);
void Compiler_usage(Compiler* self, const char* prog);

#endif