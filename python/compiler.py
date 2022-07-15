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

    def run(self):
        tokstr = ['EOF', '+', '-', '*', '/', 'intlit']

        while self.scan():
            print('Token %s' % tokstr[self.token.type.value], end='')

            if self.token.type == TokenType.IntLit:
                print(', value %d' % self.token.intValue, end='')
            
            print('')

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