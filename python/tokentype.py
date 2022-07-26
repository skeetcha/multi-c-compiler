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
    LBrace       = 15
    RBrace       = 16
    LParen       = 17
    RParen       = 18
    # Keywords
    Print        = 19
    Int          = 20
    If           = 21
    Else         = 22