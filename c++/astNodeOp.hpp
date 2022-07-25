#pragma once

enum ASTNodeOp {
    A_Add=1, A_Subtract, A_Multiply, A_Divide,
    A_Equal, A_NotEqual, A_LessThan, A_GreaterThan, A_LessEqual, A_GreaterEqual,
    A_IntLit,
    A_LVIdent, A_Assign, A_Ident
};