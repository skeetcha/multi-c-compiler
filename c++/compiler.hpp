#pragma once
#include <fstream>
#include "token.hpp"
#include <string>
#include "astNodeOp.hpp"
#include "astNode.hpp"
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
using namespace std;
using namespace llvm;

class Compiler {
private:
    ifstream inFile;
    int line;
    char putback;
    Token token;

    char next();
    char skip();
    bool scan();
    int scanint(char c);
    string scanident(char c);
    TokenType keyword(string s);

    void match(TokenType ttype, string tstr);
    void semi();

    ASTNodeOp arithop(TokenType tok);
    ASTNode* number();
    ASTNode* expr();
    ASTNode* add_expr();
    ASTNode* mul_expr();
    void statements(IRBuilder<>* builder, FunctionType* printf_type, Function* printf_fn, LLVMContext* context);
    Value* buildAST(ASTNode* node, IRBuilder<>* builder);
    void generatePrint(IRBuilder<>* builder, Value* val, FunctionType* printf_type, Function* printf_fn, LLVMContext* context);
    void parse();
public:
    Compiler(string filename);
    void run();
};