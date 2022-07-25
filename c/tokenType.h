#ifndef TOKENTYPE_H
#define TOKENTYPE_H

typedef enum {
    T_EOF,
    T_PLUS, T_MINUS,
    T_STAR, T_SLASH,
    T_EQUAL, T_NOTEQUAL,
    T_LESSTHAN, T_GREATERTHAN, T_LESSEQUAL, T_GREATEREQUAL,
    T_INTLIT, T_SEMI, T_ASSIGN, T_IDENT,
    // Keywords
    T_PRINT, T_INT
} TokenType;

#endif