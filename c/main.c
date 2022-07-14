#include "compiler.h"

int main(int argc, char* argv[]) {
    Compiler compiler = Compiler_new();
    Compiler_run(&compiler, argc, argv);
    return 0;
}