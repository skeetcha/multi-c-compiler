mutable struct Compiler
    line::Int64
    putback::Char
    inFile::IOStream
    token::Token

    function Compiler(i::IOStream)
        new(1, '\n', i, Token(T_EOF, 0))
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
    else
        if isdigit(c)
            comp.token.intValue = compiler_scanint(comp, c)
            comp.token.type = T_INTLIT
            return true
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

function compiler_scanfile(comp::Compiler)
    while compiler_scan(comp)
        print("Token " * tokenToString(comp.token.type))

        if comp.token.type == T_INTLIT
            print(", value " * string(comp.token.intValue))
        end

        println("")
    end
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

    if tokenType == T_EOF
        return left
    end

    while true
        compiler_scan(comp)
        right = compiler_mulexpr(comp)
        left = ASTNode(compiler_arithop(comp, tokenType), left, right, 0)
        tokenType = comp.token.type

        if tokenType == T_EOF
            break
        end
    end

    return left
end

function compiler_mulexpr(comp::Compiler)
    left = compiler_primary(comp)
    tokenType = comp.token.type

    if tokenType == T_EOF
        return left
    end

    while (tokenType == T_STAR) || (tokenType == T_SLASH)
        compiler_scan(comp)
        right = compiler_primary(comp)
        left = ASTNode(compiler_arithop(comp, tokenType), left, right, 0)
        tokenType = comp.token.type

        if tokenType == T_EOF
            break
        end
    end

    return left
end

function compiler_interpretAST(node::ASTNode)
    leftVal = 0
    rightVal = 0

    if node.left != nothing
        leftVal = compiler_interpretAST(node.left)
    end

    if node.right != nothing
        rightVal = compiler_interpretAST(node.right)
    end

    if node.op == A_ADD
        return leftVal + rightVal
    elseif node.op == A_SUBTRACT
        return leftVal - rightVal
    elseif node.op == A_MULTIPLY
        return leftVal * rightVal
    elseif node.op == A_DIVIDE
        return convert(Int64, floor(leftVal / rightVal))
    elseif node.op == A_INTLIT
        return node.intValue
    else
        throw(ErrorException("Unknown AST operator " * string(node.op)))
    end
end