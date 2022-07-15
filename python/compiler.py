from enum import Enum
from platform import node
import sys

def assertTypes(typeList):
    for v, t, n in typeList:
        if type(v) not in t:
            if v == None and None in t:
                return
            
            raise TypeError(f'{n} is of type {str(type(v))}, not of type {str(t)}')

class Token:
    def __init__(self, ttype, intValue):
        assertTypes([(ttype, [TokenType, None], 'token'), (intValue, [int], 'intValue')])
        self.type = ttype
        self.intValue = intValue

class TokenType(Enum):
    T_EOF  = 0
    Plus   = 1
    Minus  = 2
    Star   = 3
    Slash  = 4
    IntLit = 5

class ASTNodeType(Enum):
    Add      = 0
    Subtract = 1
    Multiply = 2
    Divide   = 3
    IntLit   = 4

regList = ['%r8', '%r9', '%r10', '%r11']
freeReg = []

class ASTNode:
    def __init__(self, op, left, right, intValue):
        assertTypes([(op, [ASTNodeType, None], 'op'), (left, [ASTNode, None], 'left'), (right, [ASTNode, None], 'right'), (intValue, [int], 'intValue')])
        self.op = op
        self.left = left
        self.right = right
        self.intValue = intValue
    
    @staticmethod
    def makeLeaf(op, intValue):
        return ASTNode(op, None, None, intValue)
    
    @staticmethod
    def makeUnary(op, left, intValue):
        return ASTNode(op, left, None, intValue)

class Compiler:
    def __init__(self):
        self.putback = '\n'
        self.inFile = None
        self.line = 1
        self.token = Token(TokenType.T_EOF, 0)
        self.opPrec = [0, 10, 10, 20, 20, 0]
        self.outFile = None

    def next(self):
        c = '\0'

        if self.putback:
            c = self.putback
            self.putback = 0
            return c
        
        c = self.inFile.read(1)

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

        if c == '':
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
        k = '0123456789'.find(c) if c != '' else -1
        val = 0

        while k >= 0:
            val = val * 10 + k
            c = self.next()
            k = '0123456789'.find(c) if c != '' else -1
        
        self.putback = c
        return val
    
    def arithop(self, tok):
        if tok == TokenType.Plus:
            return ASTNodeType.Add
        elif tok == TokenType.Minus:
            return ASTNodeType.Subtract
        elif tok == TokenType.Star:
            return ASTNodeType.Multiply
        elif tok == TokenType.Slash:
            return ASTNodeType.Divide
        else:
            print('unknown token in arithop() on line %d' % self.line, file=sys.stderr)
            sys.exit(1)
    
    def primary(self):
        node = ASTNode(ASTNodeType.Add, None, None, 0)

        if self.token.type == TokenType.IntLit:
            node = ASTNode.makeLeaf(ASTNodeType.IntLit, self.token.intValue)
            self.scan()
            return node
        else:
            print('syntax error on line %d' % self.line, file=sys.stderr)
            sys.exit(1)
    
    def additive_expr(self):
        left = self.multiplicative_expr()
        tokenType = self.token.type

        if tokenType == TokenType.T_EOF:
            return left
        
        while True:
            self.scan()
            right = self.multiplicative_expr()
            left = ASTNode(self.arithop(tokenType), left, right, 0)
            tokenType = self.token.type

            if tokenType == TokenType.T_EOF:
                break
        
        return left
    
    def multiplicative_expr(self):
        left = self.primary()
        tokenType = self.token.type

        if tokenType == TokenType.T_EOF:
            return left
        
        while (tokenType == TokenType.Star) or (tokenType == TokenType.Slash):
            self.scan()
            right = self.primary()
            left = ASTNode(self.arithop(tokenType), left, right, 0)
            tokenType = self.token.type

            if tokenType == TokenType.T_EOF:
                break
        
        return left
    
    def interpretAST(self, node):
        leftVal = None
        rightVal = None
        astop = ['+', '-', '*', '/']

        if node.left != None:
            leftVal = self.interpretAST(node.left)
        
        if node.right != None:
            rightVal = self.interpretAST(node.right)
        
        if node.op == ASTNodeType.IntLit:
            print('int %d' % node.intValue)
        else:
            print('%d %s %d' % (leftVal, astop[node.op.value], rightVal))
        
        if node.op == ASTNodeType.Add:
            return leftVal + rightVal
        elif node.op == ASTNodeType.Subtract:
            return leftVal - rightVal
        elif node.op == ASTNodeType.Multiply:
            return leftVal * rightVal
        elif node.op == ASTNodeType.Divide:
            return leftVal // rightVal
        elif node.op == ASTNodeType.IntLit:
            return node.intValue
        else:
            print('Unknown AST operator %d' % node.op.value, file=sys.stderr)
            sys.exit(1)
    
    def genAST(self, node):
        leftReg = None
        rightReg = None

        if node.left != None:
            leftReg = self.genAST(node.left)
        
        if node.right != None:
            rightReg = self.genAST(node.right)
        
        if node.op == ASTNodeType.Add:
            return self.cgadd(leftReg, rightReg)
        elif node.op == ASTNodeType.Subtract:
            return self.cgsub(leftReg, rightReg)
        elif node.op == ASTNodeType.Multiply:
            return self.cgmul(leftReg, rightReg)
        elif node.op == ASTNodeType.Divide:
            return self.cgdiv(leftReg, rightReg)
        elif node.op == ASTNodeType.IntLit:
            return self.cgload(node.intValue)
        else:
            print('Unknown AST operator %d' % node.op.value, file=sys.stderr)
            sys.exit(1)
    
    def opPrecedence(self, tokenType):
        prec = self.opPrec[tokenType.value]

        if prec == 0:
            print('syntax error on line %d, token %d' % (self.line, tokenType.value), file=sys.stderr)
            sys.exit(1)
        
        return prec
    
    def binexpr(self, ptp):
        left = self.primary()
        tokenType = self.token.type

        if tokenType == TokenType.T_EOF:
            return left
        
        while self.opPrecedence(tokenType) > ptp:
            self.scan()
            right = self.binexpr(self.opPrec[tokenType.value])
            left = ASTNode(self.arithop(tokenType), left, right, 0)
            tokenType = self.token.type

            if tokenType == TokenType.T_EOF:
                return left
        
        return left
    
    def cgadd(self, r1, r2):
        self.outFile.write('\taddq\t%s, %s\n' % (regList[r1], regList[r2]))
        self.free_register(r1)
        return r2
    
    def cgsub(self, r1, r2):
        self.outFile.write('\tsubq\t%s, %s\n' % (regList[r2], regList[r1]))
        self.free_register(r2)
        return r1

    def cgmul(self, r1, r2):
        self.outFile.write('\timulq\t%s, %s\n' % (regList[r1], regList[r2]))
        self.free_register(r1)
        return r2

    def cgdiv(self, r1, r2):
        self.outFile.write('\tmovq\t%s,%%rax\n' % regList[r1])
        self.outFile.write('\tcqo\n')
        self.outFile.write('\tidivq\t%s\n' % regList[r2])
        self.outFile.write('\tmovq\t%%rax,%s\n' % regList[r1])
        self.free_register(r2)
        return r1

    def cgload(self, val):
        r = self.alloc_register()
        self.outFile.write('\tmovq\t$%d, %s\n' % (val, regList[r]))
        return r
    
    def alloc_register(self):
        for i in range(4):
            if freeReg[i]:
                freeReg[i] = 0
                return i
        
        print('Out of registers!', file=sys.stderr)
        sys.exit(1)
    
    def free_register(self, reg):
        if freeReg[reg] != 0:
            print('Error trying to free register %d' % reg, file=sys.stderr)
            sys.exit(1)
        
        freeReg[reg] = 1

    def cgprintint(self, r):
        self.outFile.write('\tmovq\t%s, %%rdi\n' % regList[r])
        self.outFile.write('\tcall\tprintint\n')
        self.free_register(r)
    
    def generateCode(self, node):
        self.cgpreamble()
        reg = self.genAST(node)
        self.cgprintint(reg)
        self.cgpostamble()
    
    def cgpreamble(self):
        self.free_all_registers()
        self.outFile.write('\t.text\n'
            + '.LC0:\n'
            + '\t.string\t"%d\\n"\n'
            + 'printint:\n'
            + '\tpushq\t%rbp\n'
            + '\tmovq\t%rsp, %rbp\n'
            + '\tsubq\t$16, %rsp\n'
            + '\tmovl\t%edi, -4(%rbp)\n'
            + '\tmovl\t-4(%rbp), %eax\n'
            + '\tmovl\t%eax, %esi\n'
            + '\tleaq\t.LC0(%rip), %rdi\n'
            + '\tmovl $0, %eax\n'
            + '\tcall printf@PLT\n'
            + '\tnop\n'
            + '\tleave\n'
            + '\tret\n'
            + '\n'
            + '\t.globl\tmain\n'
            + '\t.type\tmain, @function\n'
            + 'main:\n'
            + '\tpushq\t%rbp\n'
            + '\tmovq\t%rsp, %rbp\n')

    def cgpostamble(self):
        self.outFile.write(
            '\tmovl\t$0, %eax\n'
            + '\tpopq\t%rbp\n'
            + '\tret\n'
        )

    def free_all_registers(self):
        global freeReg
        freeReg = [1 for i in range(4)]

tokstr = ['+', '-', '*', '/', 'intlit']

def usage(prog):
    print('Usage: %s infile' % prog, file=sys.stderr)
    sys.exit(1)

def main():
    if len(sys.argv) != 2:
        usage(sys.argv[0])
    
    compiler = Compiler()
    compiler.inFile = open(sys.argv[1], 'r')
    compiler.outFile = open('out.s', 'w')
    compiler.scan()
    node = compiler.binexpr(0)
    print('%d' % compiler.interpretAST(node))
    compiler.generateCode(node)
    compiler.inFile.close()
    compiler.outFile.close()

if __name__ == '__main__':
    main()