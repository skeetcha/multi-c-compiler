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
#include <map>
using namespace std;
using namespace llvm;

class Compiler {
private:
    ifstream inFile;
    int line;
    char putback;
    Token token;
    map<string, Value*> globals;
    string text;

    char next();
    char skip();
    bool scan();
    int scanint(char c);
    string scanident(char c);
    TokenType keyword(string s);

    int op_precedence(TokenType tok);

    void match(TokenType ttype, string tstr);
    void semi();
    void ident();

    ASTNodeOp arithop(TokenType tok);
    ASTNode* primary();
    ASTNode* binexpr(int ptp);

    void statements(IRBuilder<>* builder, FunctionType* printf_type, Function* printf_fn, LLVMContext* context);
    void print_statement(IRBuilder<>* builder, LLVMContext* context, FunctionType* printf_type, Function* printf_fn);
    void var_declaration(IRBuilder<>* builder, LLVMContext* context);
    void assignment_statement(IRBuilder<>* builder, LLVMContext* context);

    void addglobal(string global_var, IRBuilder<>* builder, LLVMContext* context);
    Value* findglobal(string global_var);

    Value* buildAST(ASTNode* node, IRBuilder<>* builder, LLVMContext* context);
    void generatePrint(IRBuilder<>* builder, Value* val, FunctionType* printf_type, Function* printf_fn, LLVMContext* context);

    void parse();
public:
    Compiler(string filename);
    void run();
};