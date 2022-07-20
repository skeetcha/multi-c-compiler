#ifndef ASTNODE_H
#define ASTNODE_H

#include "astNodeOp.h"

typedef struct an {
    ASTNodeOp op;
    struct an* left;
    struct an* right;
    int intValue;
} ASTNode;

ASTNode* mkAstNode(ASTNodeOp op, ASTNode* left, ASTNode* right, int intValue);
ASTNode* mkAstLeaf(ASTNodeOp op, int intValue);
ASTNode* mkAstUnary(ASTNodeOp op, ASTNode* left, int intValue);

#endif