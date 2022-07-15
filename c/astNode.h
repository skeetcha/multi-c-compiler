#ifndef ASTNODE_H
#define ASTNODE_H

#include "astNodeType.h"

typedef struct astnode {
    ASTNodeType op;
    struct astnode* left;
    struct astnode* right;
    int intValue;
} ASTNode;

ASTNode* mkAstNode(ASTNodeType op, ASTNode* left, ASTNode* right, int intValue);
ASTNode* mkAstLeaf(ASTNodeType op, int intValue);
ASTNode* mkAstUnary(ASTNodeType op, ASTNode* left, int intValue);

#endif