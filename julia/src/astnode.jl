mutable struct ASTNode
    op::ASTNodeOp
    left::Union{ASTNode, Nothing}
    right::Union{ASTNode, Nothing}
    value::Union{Int64, LLVM.Value}

    function ASTNode(op, left, right, intValue)
        new(op, left, right, intValue)
    end

    function ASTNode(op, intValue)
        new(op, nothing, nothing, intValue)
    end

    function ASTNode(op, left, intValue)
        new(op, left, nothing, intValue)
    end
end