#include "compiler.hpp"
#include "astNodeOp.hpp"
#include "tokenType.hpp"
#include "astNode.hpp"
#include <string>
#include <cstdio>
#include <istream>
#include <cctype>
#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;

char Compiler::next() {
    char c;

    if (putback) {
        c = putback;
        putback = '\0';
        return c;
    }

    c = inFile.get();

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

int chrpos(const char* s, char c) {
    if (c == '\0') {
        return -1;
    }

    char* p = strchr(s, c);
    return (p ? p - s : -1);
}

int Compiler::scanint(char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = next();
    }

    putback = c;
    return val;
}

ASTNodeOp Compiler::arithop(TokenType tok) {
    switch (tok) {
        case TokenType::T_Plus:
            return ASTNodeOp::A_Add;
        case TokenType::T_Minus:
            return ASTNodeOp::A_Subtract;
        case TokenType::T_Star:
            return ASTNodeOp::A_Multiply;
        case TokenType::T_Slash:
            return ASTNodeOp::A_Divide;
        default:
            cerr << "Invalid token on line " << line << endl;
            exit(1);
    }
}

ASTNode* Compiler::number() {
    ASTNode* node;

    switch (token.type) {
        case TokenType::T_IntLit:
            node = new ASTNode(ASTNodeOp::A_IntLit, token.intValue);
            scan();
            return node;
        default:
            cerr << "syntax error on line " << line << endl;
            exit(1);
    }
}

ASTNode* Compiler::expr() {
    return add_expr();
}

ASTNode* Compiler::add_expr() {
    ASTNode* left = mul_expr();
    TokenType tokenType = token.type;

    if (tokenType == TokenType::T_EOF) {
        return left;
    }

    while (true) {
        scan();
        ASTNode* right = mul_expr();
        left = new ASTNode(arithop(tokenType), left, right, 0);
        tokenType = token.type;

        if (tokenType == TokenType::T_EOF) {
            break;
        }
    }

    return left;
}

ASTNode* Compiler::mul_expr() {
    ASTNode* left = number();
    TokenType tokenType = token.type;

    if (tokenType == TokenType::T_EOF) {
        return left;
    }

    while ((tokenType == TokenType::T_Star) || (tokenType == TokenType::T_Slash)) {
        scan();
        ASTNode* right = number();
        left = new ASTNode(arithop(tokenType), left, right, 0);
        tokenType = token.type;

        if (tokenType == TokenType::T_EOF) {
            break;
        }
    }

    return left;
}

const char* astop[] = {"+", "-", "*", "/"};

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

    switch (node->op) {
        case ASTNodeOp::A_IntLit:
            cout << "int " << node->intValue << endl;
            break;
        default:
            cout << leftVal << " " << astop[node->op] << " " << rightVal << endl;
            break;
    }

    switch (node->op) {
        case ASTNodeOp::A_Add:
            return leftVal + rightVal;
        case ASTNodeOp::A_Subtract:
            return leftVal - rightVal;
        case ASTNodeOp::A_Multiply:
            return leftVal * rightVal;
        case ASTNodeOp::A_Divide:
            return leftVal / rightVal;
        case ASTNodeOp::A_IntLit:
            return node->intValue;
        default:
            cerr << "unrecognized op " << node->op << " on line " << line << endl;
            exit(1);
    }
}

Compiler::Compiler(string filename) {
    inFile.open(filename);
    line = 1;
    putback = '\n';
    token = Token(TokenType::T_EOF, 0);
}

void Compiler::run() {
    scan();
    ASTNode* node = expr();
    cout << interpretAST(node) << endl;
    delete node;
    inFile.close();
}