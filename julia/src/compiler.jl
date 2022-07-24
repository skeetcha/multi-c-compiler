mutable struct Compiler
    line::Int64
    putback::Char
    inFile::IOStream
    token::Token

    function Compiler(i::IOStream)
        new(1, '\n', i, Token(-1, 0))
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
        return false
    elseif c == '+'
        comp.token.type = T_PLUS()
    elseif c == '-'
        comp.token.type = T_MINUS()
    elseif c == '*'
        comp.token.type = T_STAR()
    elseif c == '/'
        comp.token.type = T_SLASH()
    else
        if isdigit(c)
            comp.token.intValue = compiler_scanint(comp, c)
            comp.token.type = T_INTLIT()
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

function tokenToString(token::Int64)
    if token == T_PLUS()
        return "+"
    elseif token == T_MINUS()
        return "-"
    elseif token == T_STAR()
        return "*"
    elseif token == T_SLASH()
        return "/"
    elseif token == T_INTLIT()
        return "intlit"
    else
        throw(ErrorException("invalid token " * string(token)))
    end
end

function compiler_scanfile(comp::Compiler)
    while compiler_scan(comp)
        print("Token " * tokenToString(comp.token.type))

        if comp.token.type == T_INTLIT()
            print(", value " * string(comp.token.intValue))
        end

        println("")
    end
end