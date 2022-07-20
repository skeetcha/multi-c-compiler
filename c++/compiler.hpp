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

    ASTNodeOp arithop(TokenType tok);
    ASTNode* number();
    ASTNode* expr();
    ASTNode* add_expr();
    ASTNode* mul_expr();
    int interpretAST(ASTNode* node);
    Value* buildAST(ASTNode* node, IRBuilder<>* builder);
    void parse(ASTNode* node);
public:
    Compiler(string filename);
    void run();
};