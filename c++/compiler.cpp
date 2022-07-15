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
using namespace std;

int chrpos(const char* s, char c) {
    char* p;
    p = strchr(s, c);
    return (p ? p - s : -1);
}

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

bool Compiler::scan(Token& token) {
    char c = skip();

    switch (c) {
        case '\0':
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

void Compiler::scanfile() {
    Token t(TokenType::T_Plus, 0);
    const char* tokstr[] = {"+", "-", "*", "/", "intlit"};

    while (scan(t)) {
        cout << "Token " << tokstr[t.type];

        if (t.type == TokenType::T_IntLit) {
            cout << ", value " << t.intValue;
        }

        cout << endl;
    }
}