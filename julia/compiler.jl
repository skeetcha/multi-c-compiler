@enum TokenType T_None T_Plus T_Minus T_Star T_Slash T_IntLit

function TokenTypeToString(t::TokenType)
    if t == T_None
        return "None"
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

mutable struct Token
    type::TokenType
    intVal::Int64

    function Token(t::TokenType, int::Int64)
        new(t, int)
    end
end

mutable struct Compiler
    line::Int64
    putback::Char
    inFile::IOStream

    function Compiler(filename::String)
        new(1, '\n', open(filename, "r"))
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

function compiler_scan(comp::Compiler, token::Token)::Bool
    c = compiler_skip(comp)

    if c == '\0'
        return false
    elseif c == '+'
        token.type = T_Plus
    elseif c == '-'
        token.type = T_Minus
    elseif c == '*'
        token.type = T_Star
    elseif c == '/'
        token.type = T_Slash
    else
        if isdigit(c)
            token.type = T_IntLit
            token.intVal = compiler_scanint(comp, c)
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

function compiler_scanfile(comp::Compiler)
    t = Token(T_None, 0)

    while compiler_scan(comp, t)
        typeString = TokenTypeToString(t.type)
        value = t.intVal

        print("Token $typeString")

        if t.type == T_IntLit
            print("Token $value")
        end

        println("")
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
    compiler_scanfile(compiler)
    close(compiler.inFile)
end

main()