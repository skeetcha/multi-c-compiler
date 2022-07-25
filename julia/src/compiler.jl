const TEXT_LEN_LIMIT = 512

mutable struct Compiler
    line::Int64
    putback::Char
    inFile::IOStream
    token::Token
    text::String
    globals::Dict{String, LLVM.Value}

    function Compiler(i::IOStream)
        new(1, '\n', i, Token(T_EOF, 0), "", Dict())
    end
end

function compiler_next(comp::Compiler)
    c = '\0'

    if comp.putback != '\0'
        c = comp.putback
        comp.putback = '\0'
        return c
    end

    try
        c = read(comp.inFile, Char)
    catch err
        if isa(err, EOFError)
            c = '\0'
        else
            throw(err)
        end
    end

    if c == '\n'
        comp.line += 1
    end

    return c
end

function compiler_skip(comp::Compiler)
    c = compiler_next(comp)

    while (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')
        c = compiler_next(comp)
    end

    return c
end

function compiler_scan(comp::Compiler)
    c = compiler_skip(comp)

    if c == '\0'
        comp.token.type = T_EOF
        return false
    elseif c == '+'
        comp.token.type = T_PLUS
    elseif c == '-'
        comp.token.type = T_MINUS
    elseif c == '*'
        comp.token.type = T_STAR
    elseif c == '/'
        comp.token.type = T_SLASH
    elseif c == ';'
        comp.token.type = T_SEMI
    elseif c == '='
        c = compiler_next(comp)

        if c == '='
            comp.token.type = T_EQUAL
        else
            comp.putback = c
            comp.token.type = T_ASSIGN
        end
    elseif c == '!'
        c = compiler_next(comp)

        if c == '='
            comp.token.type = T_NOTEQUAL
        else
            throw(ErrorException("Unrecognized character:" * c * " on line " * string(comp.line)))
        end
    elseif c == '<'
        c = compiler_next(comp)

        if c == '='
            comp.token.type = T_LESSEQUAL
        else
            comp.putback = c
            comp.token.type = T_LESSTHAN
        end
    elseif c == '>'
        c = compiler_next(comp)

        if c == '='
            comp.token.type = T_GREATEREQUAL
        else
            comp.putback = c
            comp.token.type = T_GREATERTHAN
        end
    else
        if isdigit(c)
            comp.token.intValue = compiler_scanint(comp, c)
            comp.token.type = T_INTLIT
            return true
        elseif (isletter(c)) || (c == '_')
            comp.text = compiler_scanident(comp, c)
            tokenType = compiler_keyword(comp.text)
            
            if tokenType != nothing
                comp.token.type = tokenType
            else
                comp.token.type = T_IDENT
            end
        else
            throw(ErrorException("Unrecognized character " * c * " on line " * comp.line))
        end
    end

    return true
end

function chrpos(s::String, c::Char)
    res = findfirst(isequal(c), s)

    if res == nothing
        return -1
    end

    return res - 1
end

function compiler_scanint(comp::Compiler, c::Char)
    k = chrpos("0123456789", c)
    val = 0

    while k >= 0
        val = val * 10 + k
        c = compiler_next(comp)
        k = chrpos("0123456789", c)
    end

    comp.putback = c
    return val
end

function compiler_scanident(comp::Compiler, c::Char)
    i = 0
    buf = ""

    while (isletter(c)) || (isdigit(c)) || (c == '_')
        if i == (TEXT_LEN_LIMIT - 1)
            throw(ErrorException("identifier too long on line " * string(comp.line)))
        elseif i < (TEXT_LEN_LIMIT - 1)
            buf *= c
        end

        c = compiler_next(comp)
    end

    comp.putback = c
    buf *= '\0'
end

function compiler_keyword(s::String)
    if s == "print\0"
        return T_PRINT
    elseif s == "int\0"
        return T_INT
    end

    return nothing
end

function compiler_arithop(comp::Compiler, token::TokenType)
    if token == T_PLUS
        return A_ADD
    elseif token == T_MINUS
        return A_SUBTRACT
    elseif token == T_STAR
        return A_MULTIPLY
    elseif token == T_SLASH
        return A_DIVIDE
    elseif token == T_EQUAL
        return A_EQUAL
    elseif token == T_NOTEQUAL
        return A_NOTEQUAL
    elseif token == T_LESSTHAN
        return A_LESSTHAN
    elseif token == T_GREATERTHAN
        return A_GREATERTHAN
    elseif token == T_LESSEQUAL
        return A_LESSEQUAL
    elseif token == T_GREATEREQUAL
        return A_GREATEREQUAL
    else
        throw(ErrorException("Syntax error, token:" * string(token) * " on line " * string(comp.line)))
    end
end

function compiler_primary(comp::Compiler)
    if comp.token.type == T_INTLIT
        node = ASTNode(A_INTLIT, comp.token.intValue)
        compiler_scan(comp)
        return node
    elseif comp.token.type == T_IDENT
        id = compiler_find_global(comp, comp.text)

        if id == nothing
            throw(ErrorException("Unknown variable:" * comp.text * " on line " * string(comp.line)))
        end

        node = ASTNode(A_IDENT, id)
        compiler_scan(comp)
        return node
    else
        throw(ErrorException("Syntax error on line " * string(comp.line) * " - token " * string(comp.token.type)))
    end
end

function compiler_opPrec(comp::Compiler, tokenType::TokenType)
    if tokenType == T_EOF
        throw(ErrorException("Syntax error, token:" * string(tokenType) * " on line " * string(comp.line)))
    elseif (tokenType == T_PLUS) || (tokenType == T_MINUS)
        return 10
    elseif (tokenType == T_STAR) || (tokenType == T_SLASH)
        return 20
    elseif (tokenType == T_EQUAL) || (tokenType == T_NOTEQUAL)
        return 30
    elseif (tokenType == T_LESSTHAN) || (tokenType == T_GREATERTHAN) || (tokenType == T_LESSEQUAL) || (tokenType == T_GREATEREQUAL)
        return 40
    end
end

function compiler_binexpr(comp::Compiler, ptp::Int64)
    left = compiler_primary(comp)
    tokenType = comp.token.type

    if tokenType == T_SEMI
        return left
    end

    op_precedence = compiler_opPrec(comp, tokenType)

    while compiler_opPrec(comp, tokenType) > ptp
        compiler_scan(comp)
        right = compiler_binexpr(comp, op_precedence)
        left = ASTNode(compiler_arithop(comp, tokenType), left, right, 0)
        tokenType = comp.token.type

        if tokenType == T_SEMI
            return left
        end

        op_precedence = compiler_opPrec(comp, tokenType)
    end

    return left
end

function compiler_match(comp::Compiler, type::TokenType, str::String)
    if comp.token.type == type
        compiler_scan(comp)
    else
        throw(ErrorException(str * " expected on line " * string(comp.line)))
    end
end

function compiler_semi(comp::Compiler)
    compiler_match(comp, T_SEMI, ";")
end

function compiler_ident(comp::Compiler)
    compiler_match(comp, T_IDENT, "identifier")
end

function compiler_statements(comp::Compiler, builder::LLVM.Builder, ctx::LLVM.Context, printf_func::LLVM.Function)
    while true
        if comp.token.type == T_PRINT
            compiler_print_statement(comp, builder, ctx, printf_func)
        elseif comp.token.type == T_INT
            compiler_var_declaration(comp, builder, ctx)
        elseif comp.token.type == T_IDENT
            compiler_assignment_statement(comp, builder, ctx)
        elseif comp.token.type == T_EOF
            return
        else
            throw(ErrorException("Syntax error, token:" * string(comp.token.type) * " on line " * string(comp.line)))
        end
    end
end

function compiler_print_statement(comp::Compiler, builder::LLVM.Builder, ctx::LLVM.Context, printf_func::LLVM.Function)
    compiler_match(comp, T_PRINT, "print")
    tree = compiler_binexpr(comp, 0)
    ret_val = compiler_buildAST(comp, tree, builder, ctx)
    compiler_print(comp, ret_val, ctx, builder, printf_func)
    compiler_semi(comp)
end

function compiler_var_declaration(comp::Compiler, builder::LLVM.Builder, ctx::LLVM.Context)
    compiler_match(comp, T_INT, "int")
    compiler_ident(comp)
    compiler_add_global(comp, comp.text, builder, ctx)
    compiler_semi(comp)
end

function compiler_assignment_statement(comp::Compiler, builder::LLVM.Builder, ctx::LLVM.Context)
    compiler_ident(comp)
    id = compiler_find_global(comp, comp.text)

    if id == nothing
        throw(ErrorException("Undeclared variable:" * comp.text * " on line " * string(comp.line)))
    end

    right = ASTNode(A_LVIDENT, id)
    compiler_match(comp, T_ASSIGN, "=")
    left = compiler_binexpr(comp, 0)
    tree = ASTNode(A_ASSIGN, left, right, 0)
    compiler_buildAST(comp, tree, builder, ctx)
    compiler_semi(comp)
end

function compiler_add_global(comp::Compiler, global_var::String, builder::LLVM.Builder, ctx::LLVM.Context)
    comp.globals[global_var] = alloca!(builder, LLVM.Int32Type(ctx))
end

function compiler_find_global(comp::Compiler, global_var::String)
    return get(comp.globals, global_var, nothing)
end

function compiler_buildAST(comp::Compiler, node::ASTNode, builder::LLVM.Builder, ctx::LLVM.Context)
    leftValue = nothing
    rightValue = nothing

    if node.left != nothing
        leftValue = compiler_buildAST(comp, node.left, builder, ctx)
    end

    if node.right != nothing
        rightValue = compiler_buildAST(comp, node.right, builder, ctx)
    end

    if node.op == A_ADD
        return add!(builder, leftValue, rightValue)
    elseif node.op == A_SUBTRACT
        return sub!(builder, leftValue, rightValue)
    elseif node.op == A_MULTIPLY
        return mul!(builder, leftValue, rightValue)
    elseif node.op == A_DIVIDE
        return sdiv!(builder, leftValue, rightValue)
    elseif node.op == A_INTLIT
        val = alloca!(builder, LLVM.Int32Type(ctx))
        store!(builder, ConstantInt(LLVM.Int32Type(ctx), node.value), val)
        return load!(builder, val)
    elseif node.op == A_LVIDENT
        return node.value
    elseif node.op == A_ASSIGN
        store!(builder, leftValue, rightValue)
    elseif node.op == A_IDENT
        return load!(builder, node.value)
    elseif node.op == A_EQUAL
        val = icmp!(builder, LLVM.API.LLVMIntEQ, leftValue, rightValue)
        return zext!(builder, val, LLVM.Int32Type(ctx))
    elseif node.op == A_NOTEQUAL
        val = icmp!(builder, LLVM.API.LLVMIntNE, leftValue, rightValue)
        return zext!(builder, val, LLVM.Int32Type(ctx))
    elseif node.op == A_LESSTHAN
        val = icmp!(builder, LLVM.API.LLVMIntSLT, leftValue, rightValue)
        return zext!(builder, val, LLVM.Int32Type(ctx))
    elseif node.op == A_LESSEQUAL
        val = icmp!(builder, LLVM.API.LLVMIntSLE, leftValue, rightValue)
        return zext!(builder, val, LLVM.Int32Type(ctx))
    elseif node.op == A_GREATERTHAN
        val = icmp!(builder, LLVM.API.LLVMIntSGT, leftValue, rightValue)
        return zext!(builder, val, LLVM.Int32Type(ctx))
    elseif node.op == A_GREATEREQUAL
        val = icmp!(builder, LLVM.API.LLVMIntSGE, leftValue, rightValue)
        return zext!(builder, val, LLVM.Int32Type(ctx))
    else
        throw(ErrorException("Invalid op in buildAST " * string(node.op)))
    end
end

function compiler_print(comp::Compiler, val::LLVM.Value, ctx::LLVM.Context, builder::LLVM.Builder, printf_func::LLVM.Function)
    format_string = "%d\n\0"
    format_string_val = LLVM.ConstantArray(encode(format_string, "utf-8"); ctx)
    format_string_ptr = alloca!(builder, LLVM.ArrayType(LLVM.Int8Type(ctx), length(format_string)))
    format_string_ptr_ref = LLVM.Value(format_string_ptr.ref)
    store!(builder, format_string_val, format_string_ptr_ref)
    format_arg = bitcast!(builder, format_string_ptr_ref, LLVM.PointerType(LLVM.Int8Type(ctx)))
    call!(builder, printf_func, [format_arg, val])
end

function compiler_parse(comp::Compiler)
    @dispose ctx=Context() begin
        mod = LLVM.Module("main_module"; ctx)

        printf_param_types = [LLVM.PointerType(LLVM.Int8Type(ctx))]
        printf_ret_type = LLVM.Int32Type(ctx)
        printf_type = LLVM.FunctionType(printf_ret_type, printf_param_types; vararg=true)
        printf_func = LLVM.Function(mod, "printf", printf_type)
        linkage!(printf_func, LLVM.API.LLVMExternalLinkage)

        main_param_types = [LLVM.Int32Type(ctx), LLVM.PointerType(LLVM.PointerType(LLVM.Int8Type(ctx)))]
        main_ret_type = LLVM.Int32Type(ctx)
        main_type = LLVM.FunctionType(main_ret_type, main_param_types)
        main_func = LLVM.Function(mod, "main", main_type)

        @dispose builder=Builder(ctx) begin
            main_bb = BasicBlock(main_func, "entry"; ctx)
            position!(builder, main_bb)
            compiler_statements(comp, builder, ctx, printf_func)
            retval = alloca!(builder, LLVM.Int32Type(ctx))
            retval_ref = LLVM.Value(retval.ref)
            store!(builder, ConstantInt(LLVM.Int32Type(ctx), 0), retval_ref)
            retint = load!(builder, retval_ref)
            ret!(builder, retint)
            verify(mod)
        end

        host_triple = Sys.MACHINE
        target = LLVM.Target(triple=host_triple)
        @dispose machine=LLVM.TargetMachine(target, host_triple) begin
            LLVM.emit(machine, mod, LLVM.API.LLVMObjectFile, "output.o")
        end
    end
end