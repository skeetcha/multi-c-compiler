#include "compiler.hpp"
#include "astNodeOp.hpp"
#include "tokenType.hpp"
#include "astNode.hpp"
#include <string>
#include <cstdio>
#include <istream>
#include <cctype>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <vector>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
using namespace std;
using namespace llvm;

const int TEXT_LEN_LIMIT = 512;

char Compiler::next() {
    char c;

    if (putback) {
        c = putback;
        putback = '\0';
        return c;
    }

    c = inFile.get();

    if (c == '\n') {
        line++;
    } else if (c == EOF) {
        return '\0';
    }

    return c;
}

char Compiler::skip() {
    char c = next();

    while ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')) {
        c = next();
    }

    return c;
}

bool Compiler::scan() {
    char c = skip();

    switch (c) {
        case '\0':
            token.type = TokenType::T_EOF;
            return false;
        case '+':
            token.type = TokenType::T_Plus;
            break;
        case '-':
            token.type = TokenType::T_Minus;
            break;
        case '*':
            token.type = TokenType::T_Star;
            break;
        case '/':
            token.type = TokenType::T_Slash;
            break;
        case ';':
            token.type = TokenType::T_Semi;
            break;
        case '=':
            token.type = TokenType::T_Equals;
            break;
        default:
            if (isdigit(c)) {
                token.intValue = scanint(c);
                token.type = TokenType::T_IntLit;
                break;
            } else if ((isalpha(c)) || (c == '_')) {
                text = scanident(c);
                TokenType newTokenType = keyword(text);

                if (newTokenType != TokenType::T_EOF) {
                    token.type = newTokenType;
                } else {
                    token.type = TokenType::T_Ident;
                }

                break;
            }

            cerr << "Unrecognized character " << c << " on line " << line << endl;
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

int Compiler::scanint(char c) {
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        val = val * 10 + k;
        c = next();
    }

    putback = c;
    return val;
}

string Compiler::scanident(char c) {
    int i = 0;
    string buf = "";

    while ((isalpha(c)) || (isdigit(c)) || (c == '_')) {
        if (i == (TEXT_LEN_LIMIT - 1)) {
            cerr << "identifier too long on line " << line << endl;
            exit(1);
        } else if (i < (TEXT_LEN_LIMIT - 1)) {
            buf += c;
            i++;
        }

        c = next();
    }

    putback = c;
    return buf;
}

TokenType Compiler::keyword(string s) {
    if (s == "print") {
        return TokenType::T_Print;
    } else if (s == "int") {
        return TokenType::T_Int;
    }

    return TokenType::T_EOF;
}

ASTNodeOp Compiler::arithop(TokenType tok) {
    switch (tok) {
        case TokenType::T_Plus:
            return ASTNodeOp::A_Add;
        case TokenType::T_Minus:
            return ASTNodeOp::A_Subtract;
        case TokenType::T_Star:
            return ASTNodeOp::A_Multiply;
        case TokenType::T_Slash:
            return ASTNodeOp::A_Divide;
        default:
            cerr << "Invalid token on line " << line << endl;
            exit(1);
    }
}

void Compiler::match(TokenType ttype, string tstr) {
    if (token.type == ttype) {
        scan();
    } else {
        cerr << tstr << " expected on line " << line << endl;
        exit(1);
    }
}

void Compiler::semi() {
    match(TokenType::T_Semi, ";");
}

void Compiler::ident() {
    match(TokenType::T_Ident, "identifier");
}

ASTNode* Compiler::primary() {
    ASTNode* node;
    Value* id;

    switch (token.type) {
        case TokenType::T_IntLit:
            node = new ASTNode(ASTNodeOp::A_IntLit, (int)token.intValue);
            scan();
            return node;
        case TokenType::T_Ident:
            id = findglobal(text);

            if (id == nullptr) {
                cerr << "Unknown variable" << ":" << text << " on line " << line << endl;
                exit(1);
            }

            node = new ASTNode(ASTNodeOp::A_Ident, id);
            scan();
            return node;
        default:
            cerr << "syntax error on line " << line << endl;
            exit(1);
    }
}

ASTNode* Compiler::expr() {
    return add_expr();
}

ASTNode* Compiler::add_expr() {
    ASTNode* left = mul_expr();
    TokenType tokenType = token.type;

    if (tokenType == TokenType::T_Semi) {
        return left;
    }

    while (true) {
        scan();
        ASTNode* right = mul_expr();
        left = new ASTNode(arithop(tokenType), left, right, 0);
        tokenType = token.type;

        if (tokenType == TokenType::T_Semi) {
            break;
        }
    }

    return left;
}

ASTNode* Compiler::mul_expr() {
    ASTNode* left = primary();
    TokenType tokenType = token.type;

    if (tokenType == TokenType::T_Semi) {
        return left;
    }

    while ((tokenType == TokenType::T_Star) || (tokenType == TokenType::T_Slash)) {
        scan();
        ASTNode* right = primary();
        left = new ASTNode(arithop(tokenType), left, right, 0);
        tokenType = token.type;

        if (tokenType == TokenType::T_Semi) {
            break;
        }
    }

    return left;
}

void Compiler::statements(IRBuilder<>* builder, FunctionType* printf_type, Function* printf_fn, LLVMContext* context) {
    while (true) {
        switch (token.type) {
            case TokenType::T_Print:
                print_statement(builder, context, printf_type, printf_fn);
                break;
            case TokenType::T_Int:
                var_declaration(builder, context);
                break;
            case TokenType::T_Ident:
                assignment_statement(builder, context);
                break;
            case TokenType::T_EOF:
                return;
            default:
                cerr << "Syntax error, token" << ":" << token.type << " on line " << line << endl;
                exit(1);
        }
    }
}

void Compiler::print_statement(IRBuilder<>* builder, LLVMContext* context, FunctionType* printf_type, Function* printf_fn) {
    match(TokenType::T_Print, "print");
    ASTNode* tree = expr();
    Value* ret_val = buildAST(tree, builder, context);
    generatePrint(builder, ret_val, printf_type, printf_fn, context);
    delete tree;
    semi();
}

void Compiler::var_declaration(IRBuilder<>* builder, LLVMContext* context) {
    match(T_Int, "int");
    ident();
    addglobal(text, builder, context);
    semi();
}

void Compiler::assignment_statement(IRBuilder<>* builder, LLVMContext* context) {
    ident();
    Value* id;

    if ((id = findglobal(text)) == nullptr) {
        cerr << "Undeclared variable" << ":" << text << " on line " << line << endl;
        exit(1);
    }

    ASTNode* right = new ASTNode(ASTNodeOp::A_LVIdent, id);
    match(T_Equals, "=");
    ASTNode* left = expr();
    ASTNode* tree = new ASTNode(A_Assign, left, right, (int)0);
    buildAST(tree, builder, context);
    delete tree;
    semi();
}

void Compiler::addglobal(string global_var, IRBuilder<>* builder, LLVMContext* context) {
    AllocaInst* inst = builder->CreateAlloca(Type::getInt32Ty(*context), 5);
    globals.insert(pair<string, Value*>(global_var, inst));
}

Value* Compiler::findglobal(string global_var) {
    try {
        return globals.at(global_var);
    } catch (const out_of_range& err) {
        return nullptr;
    } catch (const exception& e) {
        throw e;
    }
}

Value* Compiler::buildAST(ASTNode* node, IRBuilder<>* builder, LLVMContext* context) {
    Value* leftVal;
    Value* rightVal;

    if (node->left) {
        leftVal = buildAST(node->left, builder, context);
        delete node->left;
    }

    if (node->right) {
        rightVal = buildAST(node->right, builder, context);
        delete node->right;
    }

    switch (node->op) {
        case ASTNodeOp::A_Add:
            return builder->CreateAdd(leftVal, rightVal);
        case ASTNodeOp::A_Subtract:
            return builder->CreateSub(leftVal, rightVal);
        case ASTNodeOp::A_Multiply:
            return builder->CreateMul(leftVal, rightVal);
        case ASTNodeOp::A_Divide:
            return builder->CreateSDiv(leftVal, rightVal);
        case ASTNodeOp::A_IntLit:
            return builder->getInt32((uint32_t)get<int>(node->value));
        case ASTNodeOp::A_LVIdent:
            return get<Value*>(node->value);
        case ASTNodeOp::A_Assign:
            builder->CreateStore(leftVal, rightVal);
            return nullptr;
        case ASTNodeOp::A_Ident:
            return builder->CreateLoad(Type::getInt32Ty(*context), get<Value*>(node->value));
        default:
            cerr << "unreocnigzed node in ast " << node->op << endl;
            exit(1);
    }
}

void Compiler::generatePrint(IRBuilder<>* builder, Value* val, FunctionType* printf_type, Function* printf_fn, LLVMContext* context) {
    Constant* format_const = ConstantDataArray::getString(*context, "%d\n");
    AllocaInst* var_ptr = builder->CreateAlloca(ArrayType::get(IntegerType::get(*context, 8), 4), 5);
    builder->CreateStore(format_const, var_ptr);
    Value* fmt_arg = builder->CreateBitCast(var_ptr, Type::getInt8PtrTy(*context));
    Value* printf_args[] = {fmt_arg, val};
    builder->CreateCall(printf_type, printf_fn, printf_args);
}

void Compiler::parse() {
    // Initialize everything
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    // Create context
    LLVMContext main_context;

    // Create module
    Module main_module("main_module", main_context);

    // Set module triple
    string triple = sys::getDefaultTargetTriple();
    main_module.setTargetTriple(triple);

    // Get target from triple
    string error_str;
    Target* target = (Target*)TargetRegistry::lookupTarget(triple, error_str);

    if (!target) {
        cerr << error_str << endl;
        exit(1);
    }

    // Create target machine
    string cpu_name = sys::getHostCPUName().str();
    TargetOptions opt;
    Optional<Reloc::Model> RM;
    TargetMachine* machine = target->createTargetMachine(triple, cpu_name, "", opt, RM);
    main_module.setDataLayout(machine->createDataLayout());

    // Create builder
    IRBuilder<> builder(main_context);

    // Setup printf function
    Type* printf_arg_types[] = {Type::getInt8PtrTy(main_context)};
    FunctionType* printf_type = FunctionType::get(Type::getInt32Ty(main_context), printf_arg_types, true);
    Function* printf_func = Function::Create(printf_type, Function::ExternalLinkage, 0, Twine("printf"), &main_module);

    // Setup main function
    vector<Type*> main_args;
    main_args.push_back(Type::getInt32Ty(main_context));
    main_args.push_back(PointerType::get(PointerType::get((Type*)Type::getInt8Ty(main_context), 0), 0));
    FunctionType* main_ft = FunctionType::get(Type::getInt32Ty(main_context), main_args, false);
    Function* main_function = Function::Create(main_ft, Function::CommonLinkage, "main", &main_module);
    BasicBlock* main_bb = BasicBlock::Create(main_context, "entry", main_function);
    builder.SetInsertPoint(main_bb);

    // Compile code
    statements(&builder, printf_type, printf_func, &main_context);

    // Return result from main
    builder.CreateRet(builder.getInt32(0));

    // Make sure the function is fine
    verifyFunction(*main_function);

    // Create outstream
    error_code EC;
    raw_fd_ostream dest("output.o", EC, sys::fs::OF_None);

    if (EC) {
        cerr << "Could not open file: " << EC.message() << endl;
        exit(1);
    }

    legacy::PassManager pass;
    CodeGenFileType fileType = CGFT_ObjectFile;

    if (machine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        cerr << "Target machine can't emit a file of this type" << endl;
        exit(1);
    }

    pass.run(main_module);
    dest.flush();
}

Compiler::Compiler(string filename) {
    inFile.open(filename);
    line = 1;
    putback = '\n';
    token = Token(TokenType::T_EOF, 0);
}

void Compiler::run() {
    scan();
    parse();
    inFile.close();
}