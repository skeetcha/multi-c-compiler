const TEXT_LEN_LIMIT = 512

mutable struct Compiler
    line::Int64
    putback::Char
    inFile::IOStream
    token::Token
    text::String

    function Compiler(i::IOStream)
        new(1, '\n', i, Token(T_EOF, 0), "")
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
                throw(ErrorException("Unrecognized symbol " * comp.text * " on line " * string(comp.line)))
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
    else
        throw(ErrorException("Unknown token in arithop() on line " * string(comp.line) * " - token " * string(token)))
    end
end

function compiler_primary(comp::Compiler)
    if comp.token.type == T_INTLIT
        node = ASTNode(A_INTLIT, comp.token.intValue)
        compiler_scan(comp)
        return node
    else
        throw(ErrorException("Syntax error on line " * string(comp.line) * " - token " * string(comp.token.type)))
    end
end

function compiler_binexpr(comp::Compiler)
    return compiler_addexpr(comp)
end

function compiler_addexpr(comp::Compiler)
    left = compiler_mulexpr(comp)
    tokenType = comp.token.type

    if tokenType == T_SEMI
        return left
    end

    while true
        compiler_scan(comp)
        right = compiler_mulexpr(comp)
        left = ASTNode(compiler_arithop(comp, tokenType), left, right, 0)
        tokenType = comp.token.type

        if tokenType == T_SEMI
            break
        end
    end

    return left
end

function compiler_mulexpr(comp::Compiler)
    left = compiler_primary(comp)
    tokenType = comp.token.type

    if tokenType == T_SEMI
        return left
    end

    while (tokenType == T_STAR) || (tokenType == T_SLASH)
        compiler_scan(comp)
        right = compiler_primary(comp)
        left = ASTNode(compiler_arithop(comp, tokenType), left, right, 0)
        tokenType = comp.token.type

        if tokenType == T_SEMI
            break
        end
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

function compiler_statements(comp::Compiler, builder::LLVM.Builder, ctx::LLVM.Context, printf_func::LLVM.Function)
    while true
        compiler_match(comp, T_PRINT, "print")
        tree = compiler_binexpr(comp)
        val = compiler_buildAST(comp, tree, builder, ctx)
        compiler_print(comp, val, ctx, builder, printf_func)
        compiler_semi(comp)

        if comp.token.type == T_EOF
            return
        end
    end
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
        store!(builder, ConstantInt(LLVM.Int32Type(ctx), node.intValue), val)
        return load!(builder, val)
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