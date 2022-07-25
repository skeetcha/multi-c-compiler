#ifndef ASTNODEOP_H
#define ASTNODEOP_H

typedef enum {
    A_ADD,
    A_SUBTRACT,
    A_MULTIPLY,
    A_DIVIDE,
    A_INTLIT,
    A_LVIDENT,
    A_ASSIGN,
    A_IDENT
} ASTNodeOp;

#endif