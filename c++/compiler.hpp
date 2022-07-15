#pragma once
#include <vector>
#include <string>
#include "token.hpp"
#include <fstream>
#include "astNodeType.hpp"
#include "tokenType.hpp"
#include "astNode.hpp"
using namespace std;

class Compiler {
private:
    int line;
    char putback;
    ifstream inFile;
    Token token;

    char next();
    char skip();
    int scanint(char c);
    ASTNodeType arithop(TokenType tok);
    ASTNode* primary();
    int op_precedence(TokenType tok);
public:
    Compiler(string filename);
    bool scan();
    ASTNode* binexpr(int ptp);
    int interpretAST(ASTNode* node);
};