#include <iostream>
#include <cstdlib>
#include "compiler.hpp"
#include "astNode.hpp"
using namespace std;

void usage(char* prog) {
    cerr << "Usage: " << prog << " infile" << endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    Compiler compiler(argv[1]);
    compiler.scan();
    ASTNode* node = compiler.binexpr();
    cout << compiler.interpretAST(node) << endl;
    delete node;
    return 0;
}