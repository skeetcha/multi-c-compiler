class ASTNode:
    def __init__(self, op, left, mid, right, value):
        self.op = op
        self.left = left
        self.mid = mid
        self.right = right
        self.value = value
    
    @staticmethod
    def mkAstLeaf(op, value):
        return ASTNode(op, None, None, None, value)
    
    @staticmethod
    def mkAstUnary(op, left, value):
        return ASTNode(op, left, None, None, value)