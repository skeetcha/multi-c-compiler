from enum import Enum
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
    Plus   = 0
    Minus  = 1
    Star   = 2
    Slash  = 3
    IntLit = 4

class Compiler:
    def __init__(self):
        self.putback = '\n'
        self.inFile = None
        self.line = 1

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
    
    def scan(self, token):
        assertTypes([(token, [Token], 'token')])
        c = self.skip()

        if c == '':
            return False
        elif c == '+':
            token.type = TokenType.Plus
        elif c == '-':
            token.type = TokenType.Minus
        elif c == '*':
            token.type = TokenType.Star
        elif c == '/':
            token.type = TokenType.Slash
        else:
            if c.isdigit():
                token.intValue = self.scanint(c)
                token.type = TokenType.IntLit
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
    
    def scanfile(self):
        t = Token(None, 0)
        
        while self.scan(t):
            print('Token %s' % tokstr[t.type.value], end='')

            if t.type == TokenType.IntLit:
                print(', value %d' % t.intValue, end='')
            
            print('')

tokstr = ['+', '-', '*', '/', 'intlit']

def usage(prog):
    print('Usage: %s infile' % prog, file=sys.stderr)
    sys.exit(1)

def main():
    if len(sys.argv) != 2:
        usage(sys.argv[0])
    
    compiler = Compiler()
    compiler.inFile = open(sys.argv[1], 'r')
    compiler.scanfile()
    compiler.inFile.close()

if __name__ == '__main__':
    main()