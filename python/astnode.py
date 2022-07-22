class ASTNode:
    def __init__(self, op, left, right, value):
        self.op = op
        self.left = left
        self.right = right
        self.value = value
    
    @staticmethod
    def mkAstLeaf(op, value):
        return ASTNode(op, None, None, value)
    
    @staticmethod
    def mkAstUnary(op, left, value):
        return ASTNode(op, left, None, value)