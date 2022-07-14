module CCompiler

@enum Tokens NoToken Plus Minus Star Slash IntLit

function TokensToString(t::Tokens)
    if t == NoToken
        return "NoToken"
    elseif t == Plus
        return "+"
    elseif t == Minus
        return "-"
    elseif t == Star
        return "*"
    elseif t == Slash
        return "/"
    elseif t == IntLit
        return "intlit"
    end
end

mutable struct Token
    token::Tokens
    intVal::Int64

    function Token(t::Tokens, int::Int64)
        new(t, int)
    end
end

Line = 1
Putback = '\n'
InFile = Nothing

function skip()
    c = next()

    while (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') || (c == '\f')
        c = next()
    end

    return c
end

function readChar(file::IOStream)
    try
        return read(file, Char)
    catch
        return '\0'
    end
end

function next()
    global Putback
    global InFile
    global Line

    c = '\0'

    if Putback != '\0'
        c = Putback
        Putback = '\0'
        return c
    end

    c = readChar(InFile)

    if c == '\n'
        Line += 1
    end

    return c
end

function putback(c::Char)
    global Putback

    Putback = c
end

function findInString(s::String, c::Char)
    result = findfirst(isequal(c), s)

    if result == nothing
        return -1
    end

    return result
end

function scanint(c::Char)
    result = findInString("0123456789", c)
    val::Int64 = 0

    while result >= 0
        val = val * 10 + (result - 1)
        c = next()
        result = findInString("0123456789", c)
    end

    putback(c)
    return val
end

function scan(t::Token)
    c = skip()

    if c == '\0'
        return false
    elseif c == '+'
        t.token = Plus
    elseif c == '-'
        t.token = Minus
    elseif c == '*'
        t.token = Star
    elseif c == '/'
        t.token = Slash
    else
        if isdigit(c)
            t.intVal = scanint(c)
            t.token = IntLit
        else
            println("Unrecognized character $c on line $Line")
            exit(1)
        end
    end

    return true
end

function scanfile()
    t = Token(NoToken, 0)

    while scan(t)
        token = TokensToString(t.token)
        print("Token $token")

        if t.token == IntLit
            int = t.intVal

            print(", value $int")
        end

        print("\n")
    end
end

function usage(prog)
    println("Usage: $prog infile")
    exit(1)
end

function __init__()
    global InFile
    
    if length(ARGS) != 1
        usage("main.jl")
    end

    InFile = open(ARGS[1])
    scanfile()
    close(InFile)
end

end # module