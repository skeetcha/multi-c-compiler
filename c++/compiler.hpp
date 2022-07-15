#pragma once
#include <vector>
#include <string>
#include "token.hpp"
#include <fstream>
using namespace std;

class Compiler {
private:
    int line;
    char putback;
    ifstream inFile;

    char next();
    char skip();
    bool scan(Token& token);
    int scanint(char c);
public:
    Compiler(string filename);
    void scanfile();
};