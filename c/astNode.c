#include "astNode.h"
#include <stdlib.h>
#include <stdio.h>

ASTNode* mkAstNode(ASTNodeType op, ASTNode* left, ASTNode* right, int intValue) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));

    if (node == NULL) {
        fprintf(stderr, "Unable to malloc in mkastnode()\n");
        exit(1);
    }

    node->op = op;
    node->left = left;
    node->right = right;
    node->intValue = intValue;
    return node;
}

ASTNode* mkAstLeaf(ASTNodeType op, int intValue) {
    return mkAstNode(op, NULL, NULL, intValue);
}

ASTNode* mkAstUnary(ASTNodeType op, ASTNode* left, int intValue) {
    return mkAstNode(op, left, NULL, intValue);
}