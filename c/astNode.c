#include "astNode.h"
#include <stdlib.h>
#include <stdio.h>
#include <llvm-c/Types.h>

ASTNode* mkAstNode_int(ASTNodeOp op, ASTNode* left, ASTNode* right, int intValue) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));

    if (!node) {
        fprintf(stderr, "Unable to create new node\n");
        exit(1);
    }

    node->op = op;
    node->left = left;
    node->right = right;
    node->intValue = intValue;
    node->nodeType = ANT_INT;

    return node;
}

ASTNode* mkAstNode_value(ASTNodeOp op, ASTNode* left, ASTNode* right, LLVMValueRef value) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));

    if (!node) {
        fprintf(stderr, "Unable to create new node\n");
        exit(1);
    }

    node->op = op;
    node->left = left;
    node->right = right;
    node->val = value;
    node->nodeType = ANT_VALUE;

    return node;
}

ASTNode* mkAstLeaf_int(ASTNodeOp op, int intValue) {
    return mkAstNode_int(op, NULL, NULL, intValue);
}

ASTNode* mkAstLeaf_value(ASTNodeOp op, LLVMValueRef value) {
    return mkAstNode_value(op, NULL, NULL, value);
}

ASTNode* mkAstUnary_int(ASTNodeOp op, ASTNode* left, int intValue) {
    return mkAstNode_int(op, left, NULL, intValue);
}

ASTNode* mkAstUnary_value(ASTNodeOp op, ASTNode* left, LLVMValueRef value) {
    return mkAstNode_value(op, left, NULL, value);
}