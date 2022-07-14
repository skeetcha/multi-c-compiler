from dataclasses import dataclass
from enum import Enum
import sys

@dataclass
class Token:
    token: int
    intValue: int

class Tokens(Enum):
    T_EOF    = 0
    T_PLUS   = 1
    T_MINUS  = 2
    T_STAR   = 3
    T_SLASH  = 4
    T_INTLIT = 5

class Compiler:
    def __init__(self):
        self.line = 1
        self.putback = '\n'
        self.inFile = None
        self.tokstr = ['EOF', '+', '-', '*', '/', 'intlit']
    
    def run(self, args):
        if len(args) != 2:
            self.usage(args[0])
        
        try:
            self.inFile = open(args[1], 'r')
        except:
            print('Unable to open %s', file=sys.stderr)
            raise
        
        self.scanfile()
        self.inFile.close()
        sys.exit(0)
    
    def usage(self, prog):
        print('Usage: %s infile' % prog, file=sys.stderr)
        sys.exit(1)

    def scanfile(self):
        t = Token(0, 0)

        while self.scan(t):
            print('Token %s' % self.tokstr[t.token.value], end='')

            if t.token == Tokens.T_INTLIT:
                print(', value %d' % t.intValue, end='')
            
            print('')
    
    def scan(self, t):
        c = self.skip()

        if c == chr(0):
            return 0
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
                t.intValue = self.scanint(c)
                t.token = Tokens.T_INTLIT
            else:
                print('Unrecognized character %c on line %d' % (c, self.line))
                sys.exit(1)
        
        return 1
    
    def skip(self):
        c = self.next()
        
        while (c == ' ') or (c == '\t') or (c == '\n') or (c == '\r') or (c == '\f'):
            c = self.next()
        
        return c

    def scanint(self, c):
        k = '0123456789'.find(c)
        val = 0

        while k >= 0:
            val = val * 10 + k
            c = self.next()
            k = '0123456789'.find(c)
        
        self.putbackVal(c)
        return val
    
    def next(self):
        c = 0

        if ord(self.putback):
            c = self.putback
            self.putback = chr(0)
            return c
        
        c = self.inFile.read(1)
        
        if c == '\n':
            self.line += 1
        elif c == '':
            return chr(0)
        
        return c

    def putbackVal(self, c):
        self.putback = c

def main():
    compiler = Compiler()
    compiler.run(sys.argv)

if __name__ == '__main__':
    main()
