@enum TokenType T_None T_EOF T_Plus T_Minus T_Star T_Slash T_IntLit
@enum ASTNodeType A_None A_Add A_Subtract A_Multiply A_Divide A_IntLit

function TokenTypeToString(t::TokenType)
    if t == T_None
        return "None"
    elseif t == T_EOF
        return "EOF"
    elseif t == T_Plus
        return "+"
    elseif t == T_Minus
        return "-"
    elseif t == T_Star
        return "*"
    elseif t == T_Slash
        return "/"
    elseif t == T_IntLit
        return "intlit"
    end
end

function ASTNodeTypeToString(a::ASTNodeType)
    if a == A_None
        return "None"
    elseif a == A_Add
        return "+"
    elseif a == A_Subtract
        return "-"
    elseif a == A_Multiply
        return "*"
    elseif a == A_Divide
        return "/"
    elseif a == A_IntLit
        return "intlit"
    end
end

mutable struct Token
    type::TokenType
    intVal::Int64

    function Token(t::TokenType, int::Int64)
        new(t, int)
    end
end

mutable struct ASTNode
    op::ASTNodeType
    left::Union{ASTNode, Nothing}
    right::Union{ASTNode, Nothing}
    intValue::Int64

    function ASTNode(op::ASTNodeType, left::Union{ASTNode, Nothing}, right::Union{ASTNode, Nothing}, intValue::Int64)
        new(op, left, right, intValue)
    end
end

function makeAstLeaf(op::ASTNodeType, intValue::Int64)::ASTNode
    return ASTNode(op, nothing, nothing, intValue)
end

function makeAstUnary(op::ASTNodeType, left::ASTNode, intValue::Int64)::ASTNode
    return ASTNode(op, left, nothing, intValue)
end

mutable struct Compiler
    line::Int64
    putback::Char
    inFile::IOStream
    token::Token

    function Compiler(filename::String)
        new(1, '\n', open(filename, "r"), Token(T_None, 0))
    end
end

function compiler_next(comp::Compiler)::Char
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

function compiler_skip(comp::Compiler)::Char
    c = compiler_next(comp)

    while (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')
        c = compiler_next(comp)
    end

    return c
end

function compiler_scan(comp::Compiler)::Bool
    c = compiler_skip(comp)

    if c == '\0'
        comp.token.type = T_EOF
        return false
    elseif c == '+'
        comp.token.type = T_Plus
    elseif c == '-'
        comp.token.type = T_Minus
    elseif c == '*'
        comp.token.type = T_Star
    elseif c == '/'
        comp.token.type = T_Slash
    else
        if isdigit(c)
            comp.token.type = T_IntLit
            comp.token.intVal = compiler_scanint(comp, c)
        else
            line = comp.line
            throw(ErrorException("Unrecognized character $c on line $line"))
        end
    end

    return true
end

function chrpos(s::String, c::Char)::Int64
    pos = findfirst(isequal(c), s)

    if pos == nothing
        return -1
    end

    return pos
end

function compiler_scanint(comp::Compiler, c::Char)::Int64
    val = 0
    k = chrpos("0123456789", c)

    while k >= 0
        val = val * 10 + (k - 1)
        c = compiler_next(comp)
        k = chrpos("0123456789", c)
    end

    comp.putback = c
    return val
end

function compiler_arithop(comp::Compiler, tok::TokenType)
    if tok == T_Plus
        return A_Add
    elseif tok == T_Minus
        return A_Subtract
    elseif tok == T_Star
        return A_Multiply
    elseif tok == T_Slash
        return A_Divide
    else
        line = comp.line
        println("unknown token in arithop() on line $line")
        exit(1)
    end
end

function compiler_primary(comp::Compiler)
    node = ASTNode(A_None, nothing, nothing, 0)

    if comp.token.type == T_IntLit
        node = makeAstLeaf(A_IntLit, comp.token.intVal)
        compiler_scan(comp)
        return node
    else
        line = comp.line
        println("syntax error on line $line")
        exit(1)
    end
end

function compiler_binexpr(comp::Compiler)
    left = compiler_primary(comp)

    if comp.token.type == T_EOF
        return left
    end

    nodeType = compiler_arithop(comp, comp.token.type)
    compiler_scan(comp)
    right = compiler_binexpr(comp)
    return ASTNode(nodeType, left, right, 0)
end

function compiler_interpretAST(comp::Compiler, node::ASTNode)::Int64
    leftVal = nothing
    rightVal = nothing

    if node.left != nothing
        leftVal = compiler_interpretAST(comp, node.left)
    end

    if node.right != nothing
        rightVal = compiler_interpretAST(comp, node.right)
    end

    if node.op == A_IntLit
        intValue = node.intValue
        println("int $intValue")
    else
        op = ASTNodeTypeToString(node.op)
        println("$leftVal $op $rightVal")
    end

    if node.op == A_Add
        return leftVal + rightVal
    elseif node.op == A_Subtract
        return leftVal - rightVal
    elseif node.op == A_Multiply
        return leftVal * rightVal
    elseif node.op == A_Divide
        return floor(leftVal / rightVal)
    elseif node.op == A_IntLit
        return node.intValue
    else
        op = node.op
        println("Unknown AST operator $op")
        exit(1)
    end
end

function usage()
    println("Usage: compiler.jl infile")
    exit(1)
end

function main()
    if length(ARGS) != 1
        usage()
    end

    compiler = Compiler(ARGS[1])
    compiler_scan(compiler)
    node = compiler_binexpr(compiler)
    result = compiler_interpretAST(compiler, node)
    println("$result")
    close(compiler.inFile)
end

main()