#include "compiler.h"
#include "compiler_private.h"
#include <string.h>
#include "tokenType.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "astNodeType.h"
#include "astNode.h"

int chrpos(const char* s, char c) {
    char* p = strchr(s, c);
    return (p ? p - s : -1);
}

const int opPrec[] = {0, 10, 10, 20, 20, 0};

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

bool Compiler_scan(Compiler* comp) {
    char c = Compiler_skip(comp);

    switch (c) {
        case '\0':
            comp->token.type = T_EOF;
            return false;
        case '+':
            comp->token.type = T_PLUS;
            break;
        case '-':
            comp->token.type = T_MINUS;
            break;
        case '*':
            comp->token.type = T_STAR;
            break;
        case '/':
            comp->token.type = T_SLASH;
            break;
        default:
            if (isdigit(c)) {
                comp->token.intValue = Compiler_scanint(comp, c);
                comp->token.type = T_INTLIT;
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

ASTNodeType Compiler_arithop(Compiler* comp, TokenType tok) {
    switch (tok) {
        case T_PLUS:
            return A_ADD;
        case T_MINUS:
            return A_SUBTRACT;
        case T_STAR:
            return A_MULTIPLY;
        case T_SLASH:
            return A_DIVIDE;
        default:
            fprintf(stderr, "unknown token in arithop() on line %d\n", comp->line);
            exit(1);
    }
}

ASTNode* Compiler_primary(Compiler* comp) {
    ASTNode* node;

    switch (comp->token.type) {
        case T_INTLIT:
            node = mkAstLeaf(A_INTLIT, comp->token.intValue);
            Compiler_scan(comp);
            return node;
        default:
            fprintf(stderr, "syntax error on line %d\n", comp->line);
            exit(1);
    }
}

ASTNode* Compiler_binexpr(Compiler* comp, int ptp) {
    ASTNode* left = Compiler_primary(comp);
    TokenType tokenType = comp->token.type;

    if (tokenType == T_EOF) {
        return left;
    }

    while (Compiler_op_precedence(comp, tokenType) > ptp) {
        Compiler_scan(comp);
        ASTNode* right = Compiler_binexpr(comp, opPrec[tokenType]);
        left = mkAstNode(Compiler_arithop(comp, tokenType), left, right, 0);
        tokenType = comp->token.type;

        if (tokenType == T_EOF) {
            return left;
        }
    }

    return left;
}

int Compiler_interpretAST(Compiler* comp, ASTNode* node) {
    int leftVal, rightVal;

    if (node->left) {
        leftVal = Compiler_interpretAST(comp, node->left);
        free(node->left);
    }

    if (node->right) {
        rightVal = Compiler_interpretAST(comp, node->right);
        free(node->right);
    }

    const char* astop[] = {"+", "-", "*", "/"};

    if (node->op == A_INTLIT) {
        printf("int %d\n", node->intValue);
    } else {
        printf("%d %s %d\n", leftVal, astop[node->op], rightVal);
    }

    switch (node->op) {
        case A_ADD:
            return leftVal + rightVal;
        case A_SUBTRACT:
            return leftVal - rightVal;
        case A_MULTIPLY:
            return leftVal * rightVal;
        case A_DIVIDE:
            return leftVal / rightVal;
        case A_INTLIT:
            return node->intValue;
        default:
            fprintf(stderr, "Unknown AST operator %d\n", node->op);
            exit(1);
    }
}

int Compiler_op_precedence(Compiler* comp, TokenType tok) {
    int prec = opPrec[tok];

    if (prec == 0) {
        fprintf(stderr, "syntax error on line %d, token %d\n", comp->line, tok);
        exit(1);
    }

    return prec;
}