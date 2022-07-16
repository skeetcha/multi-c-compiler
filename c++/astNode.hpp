#pragma once
#include "astNodeOp.hpp"

struct ASTNode {
    ASTNodeOp op;
    ASTNode* left;
    ASTNode* right;
    int intValue;

    ASTNode() : ASTNode(ASTNodeOp::A_Add, nullptr, nullptr, 0) {}
    ASTNode(ASTNodeOp op, int intValue) : ASTNode(op, nullptr, nullptr, intValue) {}
    ASTNode(ASTNodeOp op, ASTNode* left, ASTNode* right, int intValue) : op(op), left(left), right(right), intValue(intValue) {}
};