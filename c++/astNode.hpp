#pragma once
#include "astNodeOp.hpp"
#include <llvm/IR/Value.h>
#include <variant>
using namespace std;
using namespace llvm;

typedef variant<int, Value*> ASTValue;

struct ASTNode {
    ASTNodeOp op;
    ASTNode* left;
    ASTNode* right;
    ASTValue value;

    ASTNode() : ASTNode(ASTNodeOp::A_Add, nullptr, nullptr, (int)0) {}
    ASTNode(ASTNodeOp op, ASTValue value) : ASTNode(op, nullptr, nullptr, value) {}
    ASTNode(ASTNodeOp op, ASTNode* left, ASTNode* right, ASTValue value) : op(op), left(left), right(right), value(value) {}
};