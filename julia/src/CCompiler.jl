module CCompiler

include("tokentype.jl")
include("token.jl")
include("compiler.jl")

function usage()
    println("Usage: compiler.jl infile")
    exit()
end

function __init__()
    if length(ARGS) != 1
        usage()
    end

    compiler = Compiler(open(ARGS[1], "r"))
    compiler_scanfile(compiler)
    close(compiler.inFile)
end

end # module
