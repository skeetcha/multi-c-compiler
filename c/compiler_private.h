#ifndef COMPILER_PRIVATE_H
#define COMPILER_PRIVATE_H

#include "compiler.h"
#include <stdbool.h>
#include "token.h"

char Compiler_next(Compiler* comp);
char Compiler_skip(Compiler* comp);
bool Compiler_scan(Compiler* comp, Token* token);
int Compiler_scanint(Compiler* comp, char c);

#endif