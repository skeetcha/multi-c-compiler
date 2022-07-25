#ifndef ASTNODE_H
#define ASTNODE_H

#include "astNodeOp.h"
#include <llvm-c/Types.h>

typedef enum {
    ANT_INT,
    ANT_VALUE
} ASTNodeType;

typedef struct an {
    ASTNodeOp op;
    struct an* left;
    struct an* right;
    int intValue;
    LLVMValueRef val;
    ASTNodeType nodeType;
} ASTNode;

ASTNode* mkAstNode_int(ASTNodeOp op, ASTNode* left, ASTNode* right, int intValue);
ASTNode* mkAstNode_value(ASTNodeOp op, ASTNode* left, ASTNode* right, LLVMValueRef value);
ASTNode* mkAstLeaf_int(ASTNodeOp op, int intValue);
ASTNode* mkAstLeaf_value(ASTNodeOp op, LLVMValueRef value);
ASTNode* mkAstUnary_int(ASTNodeOp op, ASTNode* left, int intValue);
ASTNode* mkAstUnary_value(ASTNodeOp op, ASTNode* left, LLVMValueRef value);

#endif