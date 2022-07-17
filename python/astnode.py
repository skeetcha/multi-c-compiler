class ASTNode:
    def __init__(self, op, left, right, intValue):
        self.op = op
        self.left = left
        self.right = right
        self.intValue = intValue
    
    @staticmethod
    def mkAstLeaf(op, intValue):
        return ASTNode(op, None, None, intValue)
    
    @staticmethod
    def mkAstUnary(op, left, intValue):
        return ASTNode(op, left, None, intValue)