#include "astNode.h"
#include <stdlib.h>
#include <stdio.h>

ASTNode* mkAstNode(ASTNodeOp op, ASTNode* left, ASTNode* right, int intValue) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));

    if (!node) {
        fprintf(stderr, "Unable to create new node\n");
        exit(1);
    }

    node->op = op;
    node->left = left;
    node->right = right;
    node->intValue = intValue;

    return node;
}

ASTNode* mkAstLeaf(ASTNodeOp op, int intValue) {
    return mkAstNode(op, NULL, NULL, intValue);
}

ASTNode* mkAstUnary(ASTNodeOp op, ASTNode* left, int intValue) {
    return mkAstNode(op, left, NULL, intValue);
}