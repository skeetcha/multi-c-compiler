#pragma once
#include "tokenType.hpp"

struct Token {
    TokenType type;
    int intValue;

    Token(TokenType type, int intValue) : type(type), intValue(intValue) {}
};