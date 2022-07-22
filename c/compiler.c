#include "compiler.h"
#include "astNodeOp.h"
#include "tokenType.h"
#include "astNode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>

const int TEXT_LEN_LIMIT = 512;

Compiler Compiler_new(char* filename) {
    Compiler comp;
    comp.inFile = fopen(filename, "r");

    if (!comp.inFile) {
        fprintf(stderr, "Unable to load file %s\n", filename);
        exit(1);
    }

    comp.line = 1;
    comp.putback = '\n';
    comp.token.type = T_EOF;
    comp.token.intValue = 0;
    return comp;
}

char Compiler_next(Compiler* comp) {
    char c;

    if (comp->putback) {
        c = comp->putback;
        comp->putback = '\0';
        return c;
    }

    c = fgetc(comp->inFile);

    if (c == '\n') {
        comp->line++;
    } else if (c == EOF) {
        c = '\0';
    }

    return c;
}

char Compiler_skip(Compiler* comp) {
    char c = Compiler_next(comp);

    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')) {
        c = Compiler_next(comp);
    }

    return c;
}

bool Compiler_scan(Compiler* comp) {
    char c = Compiler_skip(comp);

    switch (c) {
        case '\0':
            comp->token.type = T_EOF;
            return false;
        case '+':
            comp->token.type = T_PLUS;
            break;
        case '-':
            comp->token.type = T_MINUS;
            break;
        case '*':
            comp->token.type = T_STAR;
            break;
        case '/':
            comp->token.type = T_SLASH;
            break;
        case ';':
            comp->token.type = T_SEMI;
            break;
        default:
            if (isdigit(c)) {
                comp->token.intValue = Compiler_scanint(comp, c);
                comp->token.type = T_INTLIT;
                break;
            } else if ((isalpha(c)) || (c == '_')) {
                char text[512];
                Compiler_scanident(comp, c, text);
                TokenType newTokenType = Compiler_keyword(comp, text);

                if (newTokenType != T_EOF) {
                    comp->token.type = newTokenType;
                    break;
                } else {
                    fprintf(stderr, "Unrecognized symbol %s on line %d\n", text, comp->line);
                    exit(1);
                }
            }

            fprintf(stderr, "Unrecognized character %c on line %d\n", c, comp->line);
            exit(1);
    }

    return true;
}

int chrpos(char* s, char c) {
    if (c == '\0') {
        return -1;
    }

    char* p = strchr(s, c);
    return (p ? p - s : -1);
}

int Compiler_scanint(Compiler* comp, char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = Compiler_next(comp);
    }

    comp->putback = c;
    return val;
}

void Compiler_scanident(Compiler* comp, char c, char* buf) {
    int i = 0;

    while ((isalpha(c)) || (isdigit(c)) || (c == '_')) {
        if (i == (TEXT_LEN_LIMIT - 1)) {
            fprintf(stderr, "identifier too long on line %d\n", comp->line);
            exit(1);
        } else if (i < (TEXT_LEN_LIMIT - 1)) {
            buf[i++] = c;
        }

        c = Compiler_next(comp);
    }

    comp->putback = c;
    buf[i] = '\0';
}

TokenType Compiler_keyword(Compiler* comp, char* s) {
    if (s[0] == 'p') {
        if (strcmp(s, "print\0") == 0) {
            return T_PRINT;
        }
    }

    return T_EOF;
}

void Compiler_match(Compiler* comp, TokenType ttype, char* tstr) {
    if (comp->token.type == ttype) {
        Compiler_scan(comp);
    } else {
        fprintf(stderr, "%s expected on line %d\n", tstr, comp->line);
        exit(1);
    }
}

void Compiler_semi(Compiler* comp) {
    Compiler_match(comp, T_SEMI, ";");
}

ASTNodeOp Compiler_arithop(Compiler* comp, TokenType tok) {
    switch (tok) {
        case T_PLUS:
            return A_ADD;
        case T_MINUS:
            return A_SUBTRACT;
        case T_STAR:
            return A_MULTIPLY;
        case T_SLASH:
            return A_DIVIDE;
        default:
            fprintf(stderr, "Invalid token on line %d\n", comp->line);
            exit(1);
    }
}

ASTNode* Compiler_number(Compiler* comp) {
    ASTNode* node;
    
    switch (comp->token.type) {
        case T_INTLIT:
            node = mkAstLeaf(A_INTLIT, comp->token.intValue);
            Compiler_scan(comp);
            return node;
        default:
            fprintf(stderr, "Syntax error on line %d\n", comp->line);
            exit(1);
    }
}

ASTNode* Compiler_expr(Compiler* comp) {
    return Compiler_add_expr(comp);
}

ASTNode* Compiler_add_expr(Compiler* comp) {
    ASTNode* left = Compiler_mul_expr(comp);
    TokenType tokenType = comp->token.type;

    if (tokenType == T_SEMI) {
        return left;
    }

    while (true) {
        Compiler_scan(comp);
        ASTNode* right = Compiler_mul_expr(comp);
        left = mkAstNode(Compiler_arithop(comp, tokenType), left, right, 0);
        tokenType = comp->token.type;

        if (tokenType == T_SEMI) {
            break;
        }
    }

    return left;
}

ASTNode* Compiler_mul_expr(Compiler* comp) {
    ASTNode* left = Compiler_number(comp);
    TokenType tokenType = comp->token.type;

    if (tokenType == T_SEMI) {
        return left;
    }

    while ((tokenType == T_STAR) || (tokenType == T_SLASH)) {
        Compiler_scan(comp);
        ASTNode* right = Compiler_number(comp);
        left = mkAstNode(Compiler_arithop(comp, tokenType), left, right, 0);
        tokenType = comp->token.type;

        if (tokenType == T_SEMI) {
            break;
        }
    }

    return left;
}

void Compiler_statements(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context, LLVMTypeRef printf_type, LLVMValueRef printf_func) {
    while (true) {
        Compiler_match(comp, T_PRINT, "print");
        ASTNode* tree = Compiler_expr(comp);
        LLVMValueRef ret_val = Compiler_buildAST(comp, tree, builder, context);
        free(tree);
        Compiler_generatePrint(comp, builder, context, printf_type, printf_func, ret_val);
        Compiler_semi(comp);

        if (comp->token.type == T_EOF) {
            return;
        }
    }
}

LLVMValueRef Compiler_buildAST(Compiler* comp, ASTNode* node, LLVMBuilderRef builder, LLVMContextRef context) {
    LLVMValueRef leftVal;
    LLVMValueRef rightVal;

    if (node->left) {
        leftVal = Compiler_buildAST(comp, node->left, builder, context);
        free(node->left);
    }

    if (node->right) {
        rightVal = Compiler_buildAST(comp, node->right, builder, context);
        free(node->right);
    }

    switch (node->op) {
        case A_ADD:
            return LLVMBuildAdd(builder, leftVal, rightVal, "add");
        case A_SUBTRACT:
            return LLVMBuildSub(builder, leftVal, rightVal, "sub");
        case A_MULTIPLY:
            return LLVMBuildMul(builder, leftVal, rightVal, "mul");
        case A_DIVIDE:
            return LLVMBuildSDiv(builder, leftVal, rightVal, "div");
        case A_INTLIT:
            return LLVMConstInt(LLVMInt32TypeInContext(context), node->intValue, false);
        default:
            fprintf(stderr, "unrecognized node in ast %d\n", node->op);
            exit(1);
    }
}

void Compiler_generatePrint(Compiler* comp, LLVMBuilderRef builder, LLVMContextRef context, LLVMTypeRef printf_type, LLVMValueRef printf_func, LLVMValueRef value) {
    LLVMValueRef format_const = LLVMConstStringInContext(context, "%d\n", 4, true);
    LLVMValueRef var_ptr = LLVMBuildAlloca(builder, LLVMArrayType(LLVMInt8TypeInContext(context), 4), "fmt_ptr");
    LLVMBuildStore(builder, format_const, var_ptr);
    LLVMValueRef fmt_arg = LLVMBuildBitCast(builder, var_ptr, LLVMPointerType(LLVMInt8TypeInContext(context), 0), "fmt_ptr_bitcast");
    LLVMValueRef printf_args[] = {fmt_arg, value};
    LLVMBuildCall2(builder, printf_type, printf_func, printf_args, 2, "printf_res");
}

void Compiler_parse(Compiler* comp) {
    // Initialize everything
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    // Create context
    LLVMContextRef main_context = LLVMContextCreate();

    // Create module
    LLVMModuleRef main_module = LLVMModuleCreateWithNameInContext("main_module", main_context);

    // Set module triple
    char* triple = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(main_module, triple);

    // Create target from triple
    LLVMTargetRef target;
    char* errorMessage;
    LLVMBool failure = LLVMGetTargetFromTriple(triple, &target, &errorMessage);

    if (failure) {
        fprintf(stderr, "Error getting host target: %s\n", errorMessage);
        LLVMDisposeMessage(errorMessage);
        LLVMDisposeModule(main_module);
        LLVMContextDispose(main_context);
        exit(1);
    }

    // Create target machine
    char* cpu_name = LLVMGetHostCPUName();
    char* cpu_features = LLVMGetHostCPUFeatures();
    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(target, triple, cpu_name, cpu_features, LLVMCodeGenLevelNone, LLVMRelocDefault, LLVMCodeModelDefault);
    LLVMSetModuleDataLayout(main_module, LLVMCreateTargetDataLayout(machine));

    // Create builder
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(main_context);

    // Setup printf function
    LLVMTypeRef printf_arg_types[] = {LLVMPointerType(LLVMInt8TypeInContext(main_context), 0)};
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(main_context), printf_arg_types, 1, true);
    LLVMValueRef printf_func = LLVMAddFunction(main_module, "printf", printf_type);
    LLVMSetLinkage(printf_func, LLVMExternalLinkage);

    // Setup main function
    LLVMTypeRef main_arg_types[] = {LLVMInt32TypeInContext(main_context), LLVMPointerType(LLVMPointerType(LLVMInt8TypeInContext(main_context), 0), 0)};
    LLVMTypeRef main_type = LLVMFunctionType(LLVMInt32TypeInContext(main_context), main_arg_types, 2, false);
    LLVMValueRef main_func = LLVMAddFunction(main_module, "main", main_type);
    LLVMBasicBlockRef main_bb = LLVMAppendBasicBlockInContext(main_context, main_func, "entry");
    LLVMPositionBuilderAtEnd(builder, main_bb);

    // Compile code
    Compiler_statements(comp, builder, main_context, printf_type, printf_func);

    // Return result from main
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32TypeInContext(main_context), 0, false));

    // Make sure the function is fine
    LLVMVerifyFunction(main_func, LLVMAbortProcessAction);

    // Write to file
    failure = LLVMTargetMachineEmitToFile(machine, main_module, "output.o", LLVMObjectFile, &errorMessage);

    if (failure) {
        fprintf(stderr, "Error writing code to output.o: %s\n", errorMessage);
        LLVMDisposeMessage(errorMessage);
        LLVMDisposeModule(main_module);
        LLVMContextDispose(main_context);
        exit(1);
    }

    LLVMDisposeBuilder(builder);
    LLVMDisposeTargetMachine(machine);
    LLVMDisposeModule(main_module);
    LLVMContextDispose(main_context);
}

void Compiler_run(Compiler* comp) {
    Compiler_scan(comp);
    Compiler_parse(comp);
    fclose(comp->inFile);
}
