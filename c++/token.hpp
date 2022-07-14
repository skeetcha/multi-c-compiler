#pragma once
#include "tokens.hpp"

struct Token {
    Tokens token;
    int intValue;

    Token(Tokens token, int intValue) : token(token), intValue(intValue) {}
};