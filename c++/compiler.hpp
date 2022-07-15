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
public:
    Compiler(string filename);
    bool scan();
    ASTNode* binexpr();
    int interpretAST(ASTNode* node);
};