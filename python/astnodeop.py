from enum import Enum

class ASTNodeOp(Enum):
    Add          = 0
    Subtract     = 1
    Multiply     = 2
    Divide       = 3
    Equal        = 4
    NotEqual     = 5
    LessThan     = 6
    GreaterThan  = 7
    LessEqual    = 8
    GreaterEqual = 9
    IntLit       = 10
    Ident        = 11
    LVIdent      = 12
    Assign       = 13
    Glue         = 14
    If           = 15
    Print        = 16
    
    def __lt__(self, o):
        return self.value < o.value
    
    def __le__(self, o):
        return self.value <= o.value
    
    def __gt__(self, o):
        return self.value > o.value
    
    def __ge__(self, o):
        return self.value >= o.value