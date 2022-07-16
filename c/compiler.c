#include "compiler.h"
#include "astNodeOp.h"
#include "tokenType.h"
#include "astNode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

Compiler Compiler_new(char* filename) {
    Compiler comp;
    comp.inFile = fopen(filename, "r");

    if (!comp.inFile) {
        fprintf(stderr, "Unable to load file %s\n", filename);
        exit(1);
    }

    comp.line = 1;
    comp.putback = '\n';
    comp.token.type = T_EOF;
    comp.token.intValue = 0;
    return comp;
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
        c = '\0';
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

int chrpos(char* s, char c) {
    if (c == '\0') {
        return -1;
    }

    char* p = strchr(s, c);
    return (p ? p - s : -1);
}

int Compiler_scanint(Compiler* comp, char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = Compiler_next(comp);
    }

    comp->putback = c;
    return val;
}

ASTNodeOp Compiler_arithop(Compiler* comp, TokenType tok) {
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
            fprintf(stderr, "Invalid token on line %d\n", comp->line);
            exit(1);
    }
}

ASTNode* Compiler_number(Compiler* comp) {
    ASTNode* node;
    
    switch (comp->token.type) {
        case T_INTLIT:
            node = mkAstLeaf(A_INTLIT, comp->token.intValue);
            Compiler_scan(comp);
            return node;
        default:
            fprintf(stderr, "Syntax error on line %d\n", comp->line);
            exit(1);
    }
}

ASTNode* Compiler_expr(Compiler* comp) {
    return Compiler_add_expr(comp);
}

ASTNode* Compiler_add_expr(Compiler* comp) {
    ASTNode* left = Compiler_mul_expr(comp);
    TokenType tokenType = comp->token.type;

    if (tokenType == T_EOF) {
        return left;
    }

    while (true) {
        Compiler_scan(comp);
        ASTNode* right = Compiler_mul_expr(comp);
        left = mkAstNode(Compiler_arithop(comp, tokenType), left, right, 0);
        tokenType = comp->token.type;

        if (tokenType == T_EOF) {
            break;
        }
    }

    return left;
}

ASTNode* Compiler_mul_expr(Compiler* comp) {
    ASTNode* left = Compiler_number(comp);
    TokenType tokenType = comp->token.type;

    if (tokenType == T_EOF) {
        return left;
    }

    while ((tokenType == T_STAR) || (tokenType == T_SLASH)) {
        Compiler_scan(comp);
        ASTNode* right = Compiler_number(comp);
        left = mkAstNode(Compiler_arithop(comp, tokenType), left, right, 0);
        tokenType = comp->token.type;

        if (tokenType == T_EOF) {
            break;
        }
    }

    return left;
}

const char* astop[] = {"+", "-", "*", "/"};

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
            fprintf(stderr, "Unrecognized node in AST: %d\n", node->op);
            exit(1);
    }
}

void Compiler_run(Compiler* comp) {
    Compiler_scan(comp);
    ASTNode* node = Compiler_expr(comp);
    printf("%d\n", Compiler_interpretAST(comp, node));
    free(node);
    fclose(comp->inFile);
}
