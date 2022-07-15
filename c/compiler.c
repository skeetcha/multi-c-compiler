#include "compiler.h"
#include "compiler_private.h"
#include <string.h>
#include "tokenType.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int chrpos(const char* s, char c) {
    char* p = strchr(s, c);
    return (p ? p - s : -1);
}

char Compiler_next(Compiler* comp) {
    char c;

    if (comp->putback) {
        c = comp->putback;
        comp->putback = '\0';
        return c;
    }

    c = fgetc(comp->inFile);

    if (c == '\n') {
        comp->line++;
    } else if (c == EOF) {
        return '\0';
    }

    return c;
}

char Compiler_skip(Compiler* comp) {
    char c = Compiler_next(comp);

    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')) {
        c = Compiler_next(comp);
    }

    return c;
}

bool Compiler_scan(Compiler* comp, Token* token) {
    char c = Compiler_skip(comp);

    switch (c) {
        case '\0':
            return false;
        case '+':
            token->type = T_PLUS;
            break;
        case '-':
            token->type = T_MINUS;
            break;
        case '*':
            token->type = T_STAR;
            break;
        case '/':
            token->type = T_SLASH;
            break;
        default:
            if (isdigit(c)) {
                token->intValue = Compiler_scanint(comp, c);
                token->type = T_INTLIT;
                break;
            }

            fprintf(stderr, "Unrecognized character %c on line %d\n", c, comp->line);
            exit(1);
    }

    return true;
}

int Compiler_scanint(Compiler* comp, char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        if (c == '\0') {
            break;
        }

        val = val * 10 + k;
        c = Compiler_next(comp);
    }

    comp->putback = c;
    return val;
}

Compiler Compiler_new(char* filename) {
    Compiler comp;
    comp.line = 1;
    comp.putback = '\n';
    comp.inFile = fopen(filename, "r");
    return comp;
}

void Compiler_scanfile(Compiler* comp) {
    Token t;
    const char* tokstr[] = {"+", "-", "*", "/", "intlit"};

    while (Compiler_scan(comp, &t)) {
        printf("Token %s", tokstr[t.type]);

        if (t.type == T_INTLIT) {
            printf(", value %d", t.intValue);
        }

        printf("\n");
    }
}