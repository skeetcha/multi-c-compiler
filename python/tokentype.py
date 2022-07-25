from enum import Enum

# Token types
class TokenType(Enum):
    T_EOF        = 0
    Plus         = 1
    Minus        = 2
    Star         = 3
    Slash        = 4
    Equal        = 5
    NotEqual     = 6
    LessThan     = 7
    GreaterThan  = 8
    LessEqual    = 9
    GreaterEqual = 10
    IntLit       = 11
    Semi         = 12
    Assign       = 13
    Ident        = 14
    # Keywords
    Print        = 15
    Int          = 16