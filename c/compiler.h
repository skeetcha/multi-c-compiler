#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include "token.h"
#include <stdbool.h>
#include "astNodeOp.h"
#include "tokenType.h"
#include "astNode.h"
#include <llvm-c/Types.h>

typedef struct {
    FILE* inFile;
    int line;
    char putback;
    Token token;
} Compiler;

Compiler Compiler_new(char* filename);
char Compiler_next(Compiler* comp);
char Compiler_skip(Compiler* comp);
bool Compiler_scan(Compiler* comp);
int Compiler_scanint(Compiler* comp, char c);
ASTNodeOp Compiler_arithop(Compiler* comp, TokenType tok);
ASTNode* Compiler_number(Compiler* comp);
ASTNode* Compiler_expr(Compiler* comp);
ASTNode* Compiler_add_expr(Compiler* comp);
ASTNode* Compiler_mul_expr(Compiler* comp);
LLVMValueRef Compiler_buildAST(Compiler* comp, ASTNode* node, LLVMBuilderRef builder, LLVMContextRef context);
void Compiler_parse(Compiler* comp, ASTNode* node);
void Compiler_run(Compiler* comp);

#endif