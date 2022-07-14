#include "compiler.h"
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "token.h"

int chrpos(const char* s, char c) {
    char* p;
    p = strchr(s, c);
    return (p ? p - s : -1);
}

const char* tokstr[6] = { "EOF", "+", "-", "*", "/", "intlit" };

Compiler Compiler_new() {
    Compiler comp;
    comp.line = 1;
    comp.putback = '\n';
    comp.inFile = NULL;
    return comp;
}

bool Compiler_scan(Compiler* self, Token* t) {
    char c = Compiler_skip(self);

    switch (c) {
        case EOF:
            return false;
        case '+':
            t->token = T_PLUS;
            break;
        case '-':
            t->token = T_MINUS;
            break;
        case '*':
            t->token = T_STAR;
            break;
        case '/':
            t->token = T_SLASH;
            break;
        default:
            if (isdigit(c)) {
                t->intValue = Compiler_scanint(self, c);
                t->token = T_INTLIT;
                break;
            }

            fprintf(stderr, "Unrecognized character %c on line %d\n", c, self->line);
            exit(1);
    }

    return true;
}

char Compiler_skip(Compiler* self) {
    char c = Compiler_next(self);

    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')) {
        c = Compiler_next(self);
    }

    return c;
}

int Compiler_scanint(Compiler* self, char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = Compiler_next(self);
    }

    Compiler_putback(self, c);
    return val;
}

char Compiler_next(Compiler* self) {
    char c;

    if (self->putback) {
        c = self->putback;
        self->putback = '\0';
        return c;
    }

    c = fgetc(self->inFile);

    if (c == '\n') {
        self->line++;
    }

    return c;
}

void Compiler_putback(Compiler* self, char c) {
    self->putback = c;
}

void Compiler_scanfile(Compiler* self) {
    Token t;
    t.token = T_EOF;
    t.intValue = 0;

    while (Compiler_scan(self, &t)) {
        printf("Token %s", tokstr[t.token]);

        if (t.token == T_INTLIT) {
            printf(", value %d", t.intValue);
        }

        printf("\n");
    }
}

void Compiler_run(Compiler* self, int argc, char* argv[]) {
    if (argc != 2) {
        Compiler_usage(self, argv[0]);
        exit(1);
    }

    self->inFile = fopen(argv[1], "r");

    if (self->inFile == NULL) {
        fprintf(stderr, "Unable to open %s\n", argv[1]);
    }

    Compiler_scanfile(self);
}

void Compiler_usage(Compiler* self, const char* prog) {
    fprintf(stderr, "Usage: %s infile\n", prog);
}