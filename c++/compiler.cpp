#include "compiler.hpp"
#include "token.hpp"
#include <cstring>
#include "tokens.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>
using namespace std;

int chrpos(const char* s, char c) {
    char *p;

    p = strchr(s, c);
    return (p ? p - s : -1);
}

bool Compiler::scan(Token& t) {
    char c = skip();

    switch (c) {
        case EOF:
            return false;
        case '+':
            t.token = Tokens::T_PLUS;
            break;
        case '-':
            t.token = Tokens::T_MINUS;
            break;
        case '*':
            t.token = Tokens::T_STAR;
            break;
        case '/':
            t.token = Tokens::T_SLASH;
            break;
        default:
            if (isdigit(c)) {
                t.intValue = scanint(c);
                t.token = Tokens::T_INTLIT;
                break;
            }

            fprintf(stderr, "Unrecognized character %c on line %d\n", c, line);
            exit(1);
    }

    return true;
}

char Compiler::skip() {
    char c = next();

    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')) {
        c = next();
    }

    return c;
}

int Compiler::scanint(char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = next();
    }

    putbackVal(c);
    return val;
}

char Compiler::next() {
    char c;

    if (putback) {
        c = putback;
        putback = '\0';
        return c;
    }

    c = fgetc(inFile);

    if (c == '\n') {
        line++;
    }

    return c;
}

void Compiler::putbackVal(char c) {
    putback = c;
}

void Compiler::scanfile() {
    Token t(Tokens::T_EOF, 0);

    while (scan(t)) {
        printf("Token %s", tokstr[t.token]);

        if (t.token == Tokens::T_INTLIT) {
            printf(", value %d", t.intValue);
        }

        printf("\n");
    }
}

void Compiler::run(int argc, char* argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        exit(1);
    }

    inFile = fopen(argv[1], "r");

    if (inFile == nullptr) {
        fprintf(stderr, "Unable to open %s\n", argv[1]);
        exit(1);
    }

    scanfile();
}

void Compiler::usage(const char* prog) {
    fprintf(stderr, "Usage: %s infile\n", prog);
}