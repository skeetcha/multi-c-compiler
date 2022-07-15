#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"

void usage(char* prog) {
    fprintf(stderr, "Usage: %s infile\n", prog);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }

    Compiler compiler = Compiler_new(argv[1]);
    Compiler_scan(&compiler);
    ASTNode* node = Compiler_binexpr(&compiler, 0);
    printf("%d\n", Compiler_interpretAST(&compiler, node));
    free(node);
    return 0;
}