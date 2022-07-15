#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>

typedef struct {
    int line;
    char putback;
    FILE* inFile;
} Compiler;

Compiler Compiler_new(char* filename);
void Compiler_scanfile(Compiler* comp);

#endif