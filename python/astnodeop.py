from enum import Enum

class ASTNodeOp(Enum):
    Add      = 0
    Subtract = 1
    Multiply = 2
    Divide   = 3
    IntLit   = 4
    LVIdent  = 5
    Assign   = 6
    Ident    = 7