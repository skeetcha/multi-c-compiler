#ifndef COMPILER_PRIVATE_H
#define COMPILER_PRIVATE_H

#include "compiler.h"
#include "token.h"
#include "tokenType.h"
#include "astNodeType.h"

char Compiler_next(Compiler* comp);
char Compiler_skip(Compiler* comp);
int Compiler_scanint(Compiler* comp, char c);
ASTNodeType Compiler_arithop(Compiler* comp, TokenType tok);
ASTNode* Compiler_primary(Compiler* comp);
int Compiler_op_precedence(Compiler* comp, TokenType tok);

#endif