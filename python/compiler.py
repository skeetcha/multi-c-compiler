from ctypes import cdll
import sys
from llvmlite import ir
from llvmlite import binding as llvm
from ctoken import Token
from tokentype import TokenType
from astnodeop import ASTNodeOp
from astnode import ASTNode

TextLen = 512
NumSymbols = 1024

class Compiler:
    def __init__(self, filename):
        self.inFile = open(filename, 'r')
        self.line = 1
        self.putback = '\n'
        self.token = Token(TokenType.T_EOF, 0)
        self.module = ir.module.Module()
        self.main = ir.Function(self.module, ir.types.FunctionType(ir.IntType(32), [ir.IntType(32), ir.PointerType(ir.PointerType(ir.IntType(1)))]), name='main')
        bb_entry = self.main.append_basic_block()
        self.builder = ir.builder.IRBuilder()
        self.builder.position_at_end(bb_entry)
        llvm.initialize()
        llvm.initialize_native_target()
        llvm.initialize_native_asmprinter()
        printf_type = ir.FunctionType(ir.IntType(32), [ir.IntType(8).as_pointer()], var_arg=True)
        self.printf = ir.Function(self.module, printf_type, name='printf')
        self.text = ''
        self.globals = {}
    
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
        elif c == ';':
            self.token.type = TokenType.Semi
        elif c == '=':
            c = self.next()

            if c == '=':
                self.token.type = TokenType.Equal
            else:
                self.putback = c
                self.token.type = TokenType.Assign
        elif c == '!':
            c = self.next()

            if c == '=':
                self.token.type = TokenType.NotEqual
            else:
                print('Unrecognized character:%s on line %d' % (c, self.line), file=sys.stderr)
                sys.exit(1)
        elif c == '<':
            c = self.next()

            if c == '=':
                self.token.type = TokenType.LessEqual
            else:
                self.putback = c
                self.token.type = TokenType.LessThan
        elif c == '>':
            c = self.next()

            if c == '=':
                self.token.type = TokenType.GreaterEqual
            else:
                self.putback = c
                self.token.type = TokenType.GreaterThan
        else:
            if c.isdigit():
                self.token.intValue = self.scanint(c)
                self.token.type = TokenType.IntLit
            elif (c.isalpha()) or (c == '_'):
                self.text = self.scanident(c, TextLen)
                newTokenType = self.keyword(self.text)

                if newTokenType != None:
                    self.token.type = newTokenType
                else:
                    self.token.type = TokenType.Ident
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
    
    def scanident(self, c, lim):
        i = 0
        buf = ''

        while (c.isalpha()) or (c.isdigit()) or (c == '_'):
            if (lim - 1) == i:
                print('identifier too long on line %d' % self.line, file=sys.stderr)
                sys.exit(1)
            elif i < (lim - 1):
                buf += c
                i += 1
            
            c = self.next()
        
        self.putback = c
        buf += '\0'
        return buf
    
    def keyword(self, s):
        if s[0] == 'p':
            if s == 'print\0':
                return TokenType.Print
        elif s[0] == 'i':
            if s == 'int\0':
                return TokenType.Int
        
        return None
    
    def arithop(self, tokenType):
        if tokenType == TokenType.Plus:
            return ASTNodeOp.Add
        elif tokenType == TokenType.Minus:
            return ASTNodeOp.Subtract
        elif tokenType == TokenType.Star:
            return ASTNodeOp.Multiply
        elif tokenType == TokenType.Slash:
            return ASTNodeOp.Divide
        elif tokenType == TokenType.Equal:
            return ASTNodeOp.Equal
        elif tokenType == TokenType.NotEqual:
            return ASTNodeOp.NotEqual
        elif tokenType == TokenType.LessThan:
            return ASTNodeOp.LessThan
        elif tokenType == TokenType.GreaterThan:
            return ASTNodeOp.GreaterThan
        elif tokenType == TokenType.LessEqual:
            return ASTNodeOp.LessEqual
        elif tokenType == TokenType.GreaterEqual:
            return ASTNodeOp.GreaterEqual
        else:
            print('Invalid token on line %d' % self.line, file=sys.stderr)
            sys.exit(1)
    
    def primary(self):
        node = None
        id = None

        if self.token.type == TokenType.IntLit:
            node = ASTNode.mkAstLeaf(ASTNodeOp.IntLit, self.token.intValue)
        elif self.token.type == TokenType.Ident:
            id = self.findglobal(self.text)

            if id == None:
                print('Unknown variable:%s on line %d' % (self.text, self.line), file=sys.stderr)
                sys.exit(1)
            
            node = ASTNode.mkAstLeaf(ASTNodeOp.Ident, id)
        else:
            print('Syntax error, token:%d on line %d' % (self.token.type.value, self.line), file=sys.stderr)
            sys.exit(1)
        
        self.scan()
        return node
    
    def opPrec(self, tokenType):
        if tokenType == TokenType.T_EOF:
            print('Syntax error, token:%d on line %d' % (tokenType.value, self.line), file=sys.stderr)
            sys.exit(1)
        elif (tokenType == TokenType.Plus) or (tokenType == TokenType.Minus):
            return 10
        elif (tokenType == TokenType.Star) or (tokenType == TokenType.Slash):
            return 20
        elif (tokenType == TokenType.Equal) or (tokenType == TokenType.NotEqual):
            return 30
        elif (tokenType == TokenType.LessThan) or (tokenType == TokenType.GreaterThan) or (tokenType == TokenType.LessEqual) or (tokenType == TokenType.GreaterEqual):
            return 40
    
    def binexpr(self, ptp):
        left = self.primary()
        tokenType = self.token.type

        if tokenType == TokenType.Semi:
            return left
        
        op_precedence = self.opPrec(tokenType)
        
        while self.opPrec(tokenType) > ptp:
            self.scan()
            right = self.binexpr(op_precedence)
            left = ASTNode(self.arithop(tokenType), left, right, 0)
            tokenType = self.token.type

            if tokenType == TokenType.Semi:
                return left
            
            op_precedence = self.opPrec(tokenType)
        
        return left
    
    def match(self, ttype, tstr):
        if self.token.type == ttype:
            self.scan()
        else:
            print('%s expected on line %d' % (tstr, self.line), file=sys.stderr)
            sys.exit(1)
    
    def semi(self):
        self.match(TokenType.Semi, ';')
    
    def ident(self):
        self.match(TokenType.Ident, 'identifier')
    
    def statements(self):
        while True:
            if self.token.type == TokenType.Print:
                self.print_statement()
            elif self.token.type == TokenType.Int:
                self.var_declaration()
            elif self.token.type == TokenType.Ident:
                self.assignment_statement()
            elif self.token.type == TokenType.T_EOF:
                return
            else:
                print('Syntax error, token:%d on line %d' % (self.token.type.value, self.line), file=sys.stderr)
                sys.exit(1)
    
    def print_statement(self):
        self.match(TokenType.Print, 'print')
        tree = self.binexpr(0)
        ret_val = self.buildAST(tree)
        self.print('%d\n\0', ret_val)
        self.semi()

    def var_declaration(self):
        self.match(TokenType.Int, 'int')
        self.ident()
        self.addglobal(self.text)
        self.semi()

    def assignment_statement(self):
        self.ident()
        id = self.findglobal(self.text)

        if id == None:
            print('Undeclared variable:%s on line %d' % (self.text, self.line), file=sys.stderr)
            sys.exit(1)
        
        right = ASTNode.mkAstLeaf(ASTNodeOp.LVIdent, id)
        self.match(TokenType.Assign, '=')
        left = self.binexpr(0)
        tree = ASTNode(ASTNodeOp.Assign, left, right, 0)
        self.buildAST(tree)
        self.semi()
    
    def addglobal(self, text):
        self.globals[text] = self.builder.alloca(ir.types.IntType(32))

    def findglobal(self, text):
        return self.globals.get(text)
    
    def buildAST(self, node):
        leftVal = None
        rightVal = None

        if node.left != None:
            leftVal = self.buildAST(node.left)
        
        if node.right != None:
            rightVal = self.buildAST(node.right)
        
        if node.op == ASTNodeOp.Add:
            return self.builder.add(leftVal, rightVal)
        elif node.op == ASTNodeOp.Subtract:
            return self.builder.sub(leftVal, rightVal)
        elif node.op == ASTNodeOp.Multiply:
            return self.builder.mul(leftVal, rightVal)
        elif node.op == ASTNodeOp.Divide:
            return self.builder.sdiv(leftVal, rightVal)
        elif node.op == ASTNodeOp.IntLit:
            val = self.builder.alloca(ir.types.IntType(32))
            self.builder.store(ir.Constant(val.type.pointee, node.value), val)
            return self.builder.load(val)
        elif node.op == ASTNodeOp.LVIdent:
            return node.value
        elif node.op == ASTNodeOp.Assign:
            self.builder.store(leftVal, rightVal)
            return None
        elif node.op == ASTNodeOp.Equal:
            val = self.builder.icmp_signed('==', leftVal, rightVal)
            return self.builder.zext(val, ir.types.IntType(32))
        elif node.op == ASTNodeOp.NotEqual:
            val = self.builder.icmp_signed('!=', leftVal, rightVal)
            return self.builder.zext(val, ir.types.IntType(32))
        elif node.op == ASTNodeOp.LessThan:
            val = self.builder.icmp_signed('<', leftVal, rightVal)
            return self.builder.zext(val, ir.types.IntType(32))
        elif node.op == ASTNodeOp.GreaterThan:
            val = self.builder.icmp_signed('>', leftVal, rightVal)
            return self.builder.zext(val, ir.types.IntType(32))
        elif node.op == ASTNodeOp.LessEqual:
            val = self.builder.icmp_signed('<=', leftVal, rightVal)
            return self.builder.zext(val, ir.types.IntType(32))
        elif node.op == ASTNodeOp.GreaterEqual:
            val = self.builder.icmp_signed('>=', leftVal, rightVal)
            return self.builder.zext(val, ir.types.IntType(32))
        elif node.op == ASTNodeOp.Ident:
            return self.builder.load(node.value)
    
    def print(self, fmt, *vargs):
        c_fmtString_val = ir.Constant(ir.ArrayType(ir.IntType(8), len(fmt)), bytearray(fmt.encode('utf-8')))
        c_fmtString = self.builder.alloca(c_fmtString_val.type)
        self.builder.store(c_fmtString_val, c_fmtString)
        fmt_arg = self.builder.bitcast(c_fmtString, ir.IntType(8).as_pointer())
        self.builder.call(self.printf, [fmt_arg, *vargs])
    
    def parse(self):
        self.statements()
        retval = self.builder.alloca(ir.IntType(32))
        self.builder.store(ir.Constant(retval.type.pointee, 0), retval)
        retint = self.builder.load(retval)
        self.builder.ret(retint)
        mod = llvm.parse_assembly(str(self.module))
        mod.triple = llvm.get_default_triple()
        mod.verify()
        
        target = llvm.Target.from_triple(mod.triple)

        cpu_name = llvm.get_host_cpu_name()
        cpu_features = llvm.get_host_cpu_features().flatten()
        machine = target.create_target_machine(cpu=cpu_name, features=cpu_features, codemodel='default')
        data = machine.emit_object(mod)
        outFile = open('out.o', 'wb')
        outFile.write(data)
        outFile.close()

    def run(self):
        self.scan()
        self.parse()
        self.inFile.close()
