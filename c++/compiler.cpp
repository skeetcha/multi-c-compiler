#include "compiler.hpp"
#include "tokenType.hpp"
#include "token.hpp"
#include <cctype>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include "astNodeType.hpp"
#include "astNode.hpp"
using namespace std;

int chrpos(const char* s, char c) {
    char* p;
    p = strchr(s, c);
    return (p ? p - s : -1);
}

const int opPrec[] = {0, 10, 10, 20, 20, 0};

char Compiler::next() {
    char c;

    if (putback) {
        c = putback;
        putback = '\0';
        return c;
    }

    inFile.get(c);

    if (c == '\n') {
        line++;
    } else if (c == EOF) {
        return '\0';
    }

    return c;
}

char Compiler::skip() {
    char c = next();

    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')) {
        c = next();
    }

    return c;
}

bool Compiler::scan() {
    char c = skip();

    switch (c) {
        case '\0':
        case '\1':
            token.type = TokenType::T_EOF;
            return false;
        case '+':
            token.type = TokenType::T_Plus;
            break;
        case '-':
            token.type = TokenType::T_Minus;
            break;
        case '*':
            token.type = TokenType::T_Star;
            break;
        case '/':
            token.type = TokenType::T_Slash;
            break;
        default:
            if (isdigit(c)) {
                token.intValue = scanint(c);
                token.type = TokenType::T_IntLit;
                break;
            }

            cerr << "Unrecognized character " << c << " on line " << line << endl;
            exit(1);
    }

    return true;
}

int Compiler::scanint(char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        if (c == '\0') {
            break;
        }

        val = val * 10 + k;
        c = next();
    }

    putback = c;
    return val;
}

Compiler::Compiler(string filename) {
    line = 1;
    putback = '\n';
    inFile.open(filename, ifstream::in);
}

ASTNodeType Compiler::arithop(TokenType tok) {
    switch (tok) {
        case TokenType::T_Plus:
            return ASTNodeType::A_Add;
        case TokenType::T_Minus:
            return ASTNodeType::A_Subtract;
        case TokenType::T_Star:
            return ASTNodeType::A_Multiply;
        case TokenType::T_Slash:
            return ASTNodeType::A_Divide;
        default:
            cerr << "unknown token in arithop() on line " << line << endl;
            exit(1);
    }
}

ASTNode* Compiler::primary() {
    ASTNode* node;

    switch (token.type) {
        case TokenType::T_IntLit:
            node = new ASTNode(ASTNodeType::A_IntLit, token.intValue);
            scan();
            return node;
        default:
            cerr << "syntax error on line " << line << endl;
            exit(1);
    }
}

ASTNode* Compiler::binexpr(int ptp) {
    ASTNode* left = primary();
    TokenType tokenType = token.type;

    if (token.type == T_EOF) {
        return left;
    }

    while (op_precedence(tokenType) > ptp) {
        scan();
        ASTNode* right = binexpr(opPrec[tokenType]);
        left = new ASTNode(arithop(tokenType), left, right, 0);
        tokenType = token.type;

        if (tokenType == TokenType::T_EOF) {
            return left;
        }
    }

    return left;
}

int Compiler::interpretAST(ASTNode* node) {
    int leftVal, rightVal;

    if (node->left) {
        leftVal = interpretAST(node->left);
        delete node->left;
    }

    if (node->right) {
        rightVal = interpretAST(node->right);
        delete node->right;
    }

    const char* astop[] = {"+", "-", "*", "/"};

    if (node->op == ASTNodeType::A_IntLit) {
        cout << "int " << node->intValue << endl;
    } else {
        cout << leftVal << " " << astop[node->op] << " " << rightVal << endl;
    }

    switch (node->op) {
        case ASTNodeType::A_Add:
            return leftVal + rightVal;
        case ASTNodeType::A_Subtract:
            return leftVal - rightVal;
        case ASTNodeType::A_Multiply:
            return leftVal * rightVal;
        case ASTNodeType::A_Divide:
            return leftVal / rightVal;
        case ASTNodeType::A_IntLit:
            return node->intValue;
        default:
            cerr << "Unknown AST operator " << node->op << endl;
            exit(1);
    }
}

int Compiler::op_precedence(TokenType tok) {
    int prec = opPrec[tok];

    if (prec == 0) {
        cerr << "syntax error on line " << line << ", token " << tok << endl;
        exit(1);
    }

    return prec;
}