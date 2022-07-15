#pragma once
#include "astNodeType.hpp"

struct ASTNode {
    ASTNodeType op;
    ASTNode* left;
    ASTNode* right;
    int intValue;

    ASTNode(ASTNodeType op, ASTNode* left, ASTNode* right, int intValue) : op(op), left(left), right(right), intValue(intValue) {}
    ASTNode(ASTNodeType op, int intValue) : op(op), left(nullptr), right(nullptr), intValue(intValue) {}
    ASTNode(ASTNodeType op, ASTNode* left, int intValue) : op(op), left(left), right(nullptr), intValue(intValue) {}
};