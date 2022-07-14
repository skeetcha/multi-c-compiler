from ctypes import cdll
from enum import Enum
from dataclasses import dataclass
import sys

def assertTypes(typeList):
    for v, t, n in typeList:
        if type(v) not in t:
            if None in t and v == None:
                return

            raise TypeError(f'{n} is of type {str(type(v))}, not of type {str(t)}')

Line = 1
Putback = '\n'
InFile = None

class Tokens(Enum):
    T_EOF    = 0
    T_PLUS   = 1
    T_MINUS  = 2
    T_STAR   = 3
    T_SLASH  = 4
    T_INTLIT = 5

@dataclass
class Token:
    token: int
    intValue: int

CurrentToken = Token(Tokens.T_EOF, 0)

class NodeTypes(Enum):
    A_ADD      = 0
    A_SUBTRACT = 1
    A_MULTIPLY = 2
    A_DIVIDE   = 3
    A_INTLIT   = 4

class ASTNode:
    def __init__(self, op, left, right, intValue):
        assertTypes([(op, [NodeTypes], 'op'), (left, [ASTNode, None], 'left'), (right, [ASTNode, None], 'right'), (intValue, [int], 'intValue')])
        self.op = op
        self.left = left
        self.right = right
        self.intValue = intValue

def primary():
    n = None

    if CurrentToken == None:
        raise ValueError('CurrentToken is not initialized')
    
    if CurrentToken.token == Tokens.T_INTLIT:
        n = mkAstLeaf(NodeTypes.A_INTLIT, CurrentToken.intValue)
        scan(CurrentToken)
        return n
    else:
        print('syntax error on line %d' % Line, file=sys.stderr)
        sys.exit(1)

def arithop(tok):
    if tok == Tokens.T_PLUS:
        return NodeTypes.A_ADD
    elif tok == Tokens.T_MINUS:
        return NodeTypes.A_SUBTRACT
    elif tok == Tokens.T_STAR:
        return NodeTypes.A_MULTIPLY
    elif tok == Tokens.T_SLASH:
        return NodeTypes.A_DIVIDE
    else:
        print('unknown token in arithop() on line %d' % Line, file=sys.stderr)
        sys.exit(1)

def binexpr():
    n = None
    left = primary()
    right = None
    nodeType = None

    if CurrentToken.token == Tokens.T_EOF:
        return left
    
    nodeType = arithop(CurrentToken.token)
    scan(CurrentToken)
    right = binexpr()
    n = ASTNode(nodeType, left, right, 0)
    return n

ASTop = ['+', '-', '*', '/']

def interpretAST(node):
    leftVal = None
    rightVal = None

    if node.left != None:
        leftVal = interpretAST(node.left)
    
    if node.right != None:
        rightVal = interpretAST(node.right)
    
    if node.op == NodeTypes.A_INTLIT:
        print('int %d' % node.intValue)
    else:
        print('%d %s %d' % (leftVal, ASTop[node.op.value], rightVal))
    
    if node.op == NodeTypes.A_ADD:
        return leftVal + rightVal
    elif node.op == NodeTypes.A_SUBTRACT:
        return leftVal - rightVal
    elif node.op == NodeTypes.A_MULTIPLY:
        return leftVal * rightVal
    elif node.op == NodeTypes.A_DIVIDE:
        return leftVal / rightVal
    elif node.op == NodeTypes.A_INTLIT:
        return node.intValue
    else:
        print('Unknown AST operator %d' % node.op, file=sys.stderr)
        sys.exit(1)

def mkAstLeaf(op, intValue):
    return ASTNode(op, None, None, intValue)

def mkAstUnary(op, left, intValue):
    return ASTNode(op, left, None, intValue)

def scan(t):
    c = skip()

    if c == '':
        t.token = Tokens.T_EOF
        return False
    elif c == '+':
        t.token = Tokens.T_PLUS
    elif c == '-':
        t.token = Tokens.T_MINUS
    elif c == '*':
        t.token = Tokens.T_STAR
    elif c == '/':
        t.token = Tokens.T_SLASH
    else:
        if c.isdigit():
            t.intValue = scanint(c)
            t.token = Tokens.T_INTLIT
        else:
            print('Unrecognized character %s on line %d' % (c, Line))
            sys.exit(1)
    
    return True

def skip():
    c = nextChar()

    while (c == ' ') or (c == '\t') or (c == '\n') or (c == '\r') or (c == '\f'):
        c = nextChar()
    
    return c

def scanint(c):
    k = '0123456789'.find(c)
    val = 0

    while k >= 0:
        if c == '':
            break

        val = val * 10 + k
        c = nextChar()
        k = '0123456789'.find(c)
    
    putback(c)
    return val

def nextChar():
    global Putback
    global Line
    c = None

    if Putback:
        c = Putback
        Putback = 0
        return c
    
    c = InFile.read(1)

    if c == '\n':
        Line += 1
    
    return c

def putback(c):
    global Putback
    Putback = c

def usage(prog):
    print('Usage: %s infile' % prog, file=sys.stderr)
    sys.exit(1)

def main():
    global InFile
    n = None

    if len(sys.argv) != 2:
        usage(sys.argv[0])
    
    InFile = open(sys.argv[1], 'r')
    scan(CurrentToken)
    n = binexpr()
    print('%d' % interpretAST(n))
    sys.exit(0)

if __name__ == '__main__':
    main()