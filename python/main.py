import sys
from compiler import Compiler

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