#pragma once
#include <cstdio>
#include "token.hpp"
using namespace std;

class Compiler {
private:
    int line;
    char putback;
    FILE* inFile;
    const char* tokstr[6] = { "EOF", "+", "-", "*", "/", "intlit" };

    bool scan(Token& t);
    char skip();
    int scanint(char c);
    char next();
    void putbackVal(char c);
    void scanfile();
public:
    Compiler() : line(1), putback('\n'), inFile(nullptr) {}

    void run(int argc, char* argv[]);
    void usage(const char* prog);
};