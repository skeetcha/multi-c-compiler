#pragma once

enum TokenType {
    T_EOF,
    T_Plus, T_Minus,
    T_Star, T_Slash,
    T_Equal, T_NotEqual,
    T_LessThan, T_GreaterThan, T_LessEqual, T_GreaterEqual,
    T_IntLit, T_Semi, T_Assign, T_Ident,
    // Keywords
    T_Print, T_Int
};