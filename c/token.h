#ifndef TOKEN_H
#define TOKEN_H

#include "tokenType.h"

typedef struct {
    TokenType type;
    int intValue;
} Token;

#endif