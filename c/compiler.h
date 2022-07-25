#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include "token.h"
#include <stdbool.h>
#include "astNodeOp.h"
#include "tokenType.h"
#include "astNode.h"
#include <llvm-c/Types.h>
#include <glib.h>

typedef struct {
    FILE* inFile;
    int line;
    char putback;
    Token token;
    GHashTable* globals;
    char text[512];
} Compiler;

Compiler Compiler_new(char* filename);

char Compiler_next(Compiler* comp);
char Compiler_skip(Compiler* comp);
bool Compiler_scan(Compiler* comp);
int Compiler_scanint(Compiler* comp, char c);
void Compiler_scanident(Compiler* comp, char c, char* buf);
TokenType Compiler_keyword(Compiler* comp, char* s);

void Compiler_match(Compiler* comp, TokenType ttype, char* tstr);
void Compiler_semi(Compiler* comp);
void Compiler_ident(Compiler* comp);

ASTNodeOp Compiler_arithop(Compiler* comp, TokenType tok);
ASTNode* Compiler_primary(Compiler* comp);
ASTNode* Compiler_expr(Compiler* comp);
ASTNode* Compiler_add_expr(Compiler* comp);
ASTNode* Compiler_mul_expr(Compiler* comp);

void Compiler_statements(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context, LLVMTypeRef printf_type, LLVMValueRef printf_func);
void Compiler_print_statement(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context, LLVMTypeRef printf_type, LLVMValueRef printf_func);
void Compiler_var_declaration(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context);
void Compiler_assignment_statement(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context);

void Compiler_add_global(Compiler* comp, char* global_var, LLVMBuilderRef builder, LLVMContextRef context);
LLVMValueRef Compiler_find_global(Compiler* comp, char* global_var);

LLVMValueRef Compiler_buildAST(Compiler* comp, ASTNode* node, LLVMBuilderRef builder, LLVMContextRef context);
void Compiler_generatePrint(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context, LLVMTypeRef printf_type, LLVMValueRef printf_func, LLVMValueRef value);
void Compiler_parse(Compiler* comp);

void Compiler_run(Compiler* comp);

#endif