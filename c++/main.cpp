#include "compiler.hpp"
#include <iostream>
#include <cstdlib>
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
    compiler.run();
    return 0;
}