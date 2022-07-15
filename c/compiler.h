#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include "token.h"
#include "astNode.h"
#include <stdbool.h>

typedef struct {
    int line;
    char putback;
    FILE* inFile;
    Token token;
} Compiler;

Compiler Compiler_new(char* filename);
bool Compiler_scan(Compiler* comp);
ASTNode* Compiler_binexpr(Compiler* comp);
int Compiler_interpretAST(Compiler* comp, ASTNode* node);

#endif