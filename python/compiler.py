import sys
from enum import Enum

class TokenType(Enum):
    T_EOF  = 0
    Plus   = 1
    Minus  = 2
    Star   = 3
    Slash  = 4
    IntLit = 5

class Token:
    def __init__(self, tokenType, intValue):
        self.type = tokenType
        self.intValue = intValue

class ASTNodeOp(Enum):
    Add      = 0
    Subtract = 1
    Multiply = 2
    Divide   = 3
    IntLit   = 4

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

class Compiler:
    def __init__(self, filename):
        self.inFile = open(filename, 'r')
        self.line = 1
        self.putback = '\n'
        self.token = Token(TokenType.T_EOF, 0)
    
    def next(self):
        c = ''

        if self.putback != '\0':
            c = self.putback
            self.putback = '\0'
            return c
        
        try:
            c = self.inFile.read(1)
        except EOFError as e:
            return ''
        except:
            raise

        if c == '\n':
            self.line += 1
        
        return c
    
    def skip(self):
        c = self.next()

        while (c == ' ') or (c == '\t') or (c == '\n') or (c == '\r') or (c == '\f'):
            c = self.next()
        
        return c
    
    def scan(self):
        c = self.skip()

        if (c == '') or (c == '\0'):
            self.token.type = TokenType.T_EOF
            return False
        elif c == '+':
            self.token.type = TokenType.Plus
        elif c == '-':
            self.token.type = TokenType.Minus
        elif c == '*':
            self.token.type = TokenType.Star
        elif c == '/':
            self.token.type = TokenType.Slash
        else:
            if c.isdigit():
                self.token.intValue = self.scanint(c)
                self.token.type = TokenType.IntLit
            else:
                print('Unrecognized character %s on line %d' % (c, self.line), file=sys.stderr)
                sys.exit(1)
        
        return True
    
    def scanint(self, c):
        k = '0123456789'.find(c)
        val = 0

        while k >= 0:
            if (c == '') or (c == '\0'):
                break

            val = val * 10 + k
            c = self.next()
            k = '0123456789'.find(c)
        
        self.putback = c
        return val
    
    def arithop(self, tokenType):
        if tokenType == TokenType.Plus:
            return ASTNodeOp.Add
        elif tokenType == TokenType.Minus:
            return ASTNodeOp.Subtract
        elif tokenType == TokenType.Star:
            return ASTNodeOp.Multiply
        elif tokenType == TokenType.Slash:
            return ASTNodeOp.Divide
        else:
            print('Invalid token on line %d' % self.line, file=sys.stderr)
            sys.exit(1)
    
    def number(self):
        if self.token.type == TokenType.IntLit:
            node = ASTNode.mkAstLeaf(ASTNodeOp.IntLit, self.token.intValue)
            self.scan()
            return node
        else:
            print('syntax error on line %d' % self.line, file=sys.stderr)
            sys.exit(1)
    
    def expr(self):
        return self.add_expr()
    
    def add_expr(self):
        left = self.mul_expr()
        tokenType = self.token.type
        
        if tokenType == TokenType.T_EOF:
            return left
        
        while True:
            self.scan()
            right = self.mul_expr()
            left = ASTNode(self.arithop(tokenType), left, right, 0)
            tokenType = self.token.type

            if tokenType == TokenType.T_EOF:
                break
        
        return left
    
    def mul_expr(self):
        left = self.number()
        tokenType = self.token.type

        if tokenType == TokenType.T_EOF:
            return left
        
        while (tokenType == TokenType.Star) or (tokenType == TokenType.Slash):
            self.scan()
            right = self.number()
            left = ASTNode(self.arithop(tokenType), left, right, 0)
            tokenType = self.token.type

            if tokenType == TokenType.T_EOF:
                break
        
        return left
    
    def interpretAST(self, node):
        astop = ['+', '-', '*', '/']
        leftVal = 0
        rightVal = 0

        if node.left != None:
            leftVal = self.interpretAST(node.left)
        
        if node.right != None:
            rightVal = self.interpretAST(node.right)
        
        if node.op == ASTNodeOp.IntLit:
            print('int %d' % node.intValue)
        else:
            print('%d %s %d' % (leftVal, astop[node.op.value], rightVal))
        
        if node.op == ASTNodeOp.Add:
            return leftVal + rightVal
        elif node.op == ASTNodeOp.Subtract:
            return leftVal - rightVal
        elif node.op == ASTNodeOp.Multiply:
            return leftVal * rightVal
        elif node.op == ASTNodeOp.Divide:
            return leftVal // rightVal
        elif node.op == ASTNodeOp.IntLit:
            return node.intValue

    def run(self):
        self.scan()
        node = self.expr()
        print('%d' % self.interpretAST(node))
        self.inFile.close()

def usage(prog):
    print('Usage: %s infile' % prog, file=sys.stderr)
    sys.exit(1)

def main():
    if len(sys.argv) != 2:
        usage(sys.argv[0])
    
    compiler = Compiler(sys.argv[1])
    compiler.run()

if __name__ == '__main__':
    main()