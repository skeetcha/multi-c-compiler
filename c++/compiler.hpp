#pragma once
#include <fstream>
#include "token.hpp"
#include <string>
#include "astNodeOp.hpp"
#include "astNode.hpp"
using namespace std;

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
public:
    Compiler(string filename);
    void run();
};