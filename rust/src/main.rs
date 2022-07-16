use std::{env::args, process::exit, fs::File, io::Read};

#[derive(Clone)]
enum Token {
    Eof,
    Plus,
    Minus,
    Star,
    Slash,
    IntLit(i64)
}

#[derive(Clone)]
enum ASTNodeOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    IntLit(i64)
}

impl std::fmt::Display for ASTNodeOp {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Self::Add => write!(f, "+"),
            Self::Subtract => write!(f, "-"),
            Self::Multiply => write!(f, "*"),
            Self::Divide => write!(f, "/"),
            Self::IntLit(val) => write!(f, "int {}", val)
        }
    }
}

#[derive(Clone)]
struct ASTNode {
    op: ASTNodeOp,
    left: Option<Box<ASTNode>>,
    right: Option<Box<ASTNode>>
}

impl ASTNode {
    fn new(op: ASTNodeOp, left: Box<ASTNode>, right: Box<ASTNode>) -> Self {
        Self {
            op: op,
            left: Some(left),
            right: Some(right)
        }
    }

    fn mkAstLeaf(op: ASTNodeOp) -> Self {
        Self {
            op: op,
            left: None,
            right: None
        }
    }

    fn mkAstUnary(op: ASTNodeOp, left: Box<ASTNode>) -> Self {
        Self {
            op: op,
            left: Some(left),
            right: None
        }
    }
}

fn chrpos(s: &str, c: char) -> i64 {
    match s.chars().position(|ch| ch == c) {
        Some(val) => val as i64,
        None => -1
    }
}

struct Compiler {
    in_data: Vec<u8>,
    index: usize,
    line: i64,
    putback: char,
    token: Token
}

impl Compiler {
    fn next(&mut self) -> char {
        let mut c: char = '\0';

        if self.putback != '\0' {
            c = self.putback;
            self.putback = '\0';
            return c;
        }

        c = self.read();

        if c == '\n' {
            self.line += 1;
        }

        c
    }

    fn read(&mut self) -> char {
        if self.index < self.in_data.len() {
            let c: char = self.in_data[self.index] as char;
            self.index += 1;
            c
        } else {
            '\0'
        }
    }

    fn skip(&mut self) -> char {
        let mut c: char = self.next();

        while (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') {
            c = self.next();
        }

        c
    }

    fn scan(&mut self) -> bool {
        let c: char = self.skip();

        match c {
            '\0' => {
                self.token = Token::Eof;
                false
            },
            '+' => {
                self.token = Token::Plus;
                true
            },
            '-' => {
                self.token = Token::Minus;
                true
            },
            '*' => {
                self.token = Token::Star;
                true
            },
            '/' => {
                self.token = Token::Slash;
                true
            },
            _ => {
                if c.is_digit(10) {
                    self.token = Token::IntLit(self.scanint(c));
                    true
                } else {
                    panic!("Unrecognized character {} on line {}", c, self.line);
                }
            }
        }
    }

    fn scanint(&mut self, c: char) -> i64 {
        let mut k: i64 = chrpos("0123456789", c);
        let mut val: i64 = 0;
        let mut ch: char = c;

        while k >= 0 {
            if ch == '\0' {
                break;
            }

            val = val * 10 + k;
            ch = self.next();
            k = chrpos("0123456789", ch);
        }

        self.putback = ch;
        val
    }

    fn new(filename: &str) -> Self {
        let mut f = File::open(filename).expect("no file found");
        let metadata = std::fs::metadata(filename).expect("unable to read metadata");
        let mut buffer = vec![0; metadata.len() as usize];
        f.read(&mut buffer).expect("buffer overflow");
        
        Self {
            line: 1,
            putback: '\n',
            in_data: buffer.to_owned(),
            index: 0,
            token: Token::Eof
        }
    }

    fn arithop(&self, token: Token) -> ASTNodeOp {
        match token {
            Token::Plus => ASTNodeOp::Add,
            Token::Minus => ASTNodeOp::Subtract,
            Token::Star => ASTNodeOp::Multiply,
            Token::Slash => ASTNodeOp::Divide,
            _ => {
                panic!("Invalid token on line {}", self.line);
            }
        }
    }

    fn number(&mut self) -> ASTNode {
        if let Token::IntLit(val) = self.token {
            let node = ASTNode::mkAstLeaf(ASTNodeOp::IntLit(val));
            self.scan();
            node
        } else {
            panic!("syntax error on line {}", self.line);
        }
    }

    fn expr(&mut self) -> ASTNode {
        self.add_expr()
    }

    fn add_expr(&mut self) -> ASTNode {
        let mut left: ASTNode = self.mul_expr();
        let mut token: Token = self.token.clone();

        if let Token::Eof = token.clone() {
            return left;
        }

        while true {
            self.scan();
            let right = self.mul_expr();
            left = ASTNode::new(self.arithop(token), Box::new(left), Box::new(right));
            token = self.token.clone();

            if let Token::Eof = token.clone() {
                break;
            }
        }

        left
    }

    fn mul_expr(&mut self) -> ASTNode {
        let mut left: ASTNode = self.number();
        let mut token: Token = self.token.clone();

        if let Token::Eof = token.clone() {
            return left;
        }

        while true {
            match token.clone() {
                Token::Eof => break,
                Token::Plus => break,
                Token::Minus => break,
                Token::Star => (),
                Token::Slash => (),
                Token::IntLit(_) => break
            }

            self.scan();
            let right = self.number();
            left = ASTNode::new(self.arithop(token), Box::new(left), Box::new(right));
            token = self.token.clone();

            if let Token::Eof = token.clone() {
                break;
            }
        }

        left
    }

    fn interpret_ast(&mut self, node: ASTNode) -> i64 {
        let mut left_val: i64 = 0;
        let mut right_val: i64 = 0;

        if let Some(left_node) = node.left.clone() {
            left_val = self.interpret_ast(*left_node);
        }

        if let Some(right_node) = node.right.clone() {
            right_val = self.interpret_ast(*right_node);
        }

        if let ASTNodeOp::IntLit(val) = node.op.clone() {
            println!("int {}", val);
        } else {
            println!("{} {} {}", left_val, node.op.clone(), right_val);
        }

        match node.op.clone() {
            ASTNodeOp::Add => left_val + right_val,
            ASTNodeOp::Subtract => left_val - right_val,
            ASTNodeOp::Multiply => left_val * right_val,
            ASTNodeOp::Divide => left_val / right_val,
            ASTNodeOp::IntLit(val) => val
        }
    }

    fn run(&mut self) {
        self.scan();
        let node = self.expr();
        println!("{}", self.interpret_ast(node));
    }
}

fn usage(prog: String) {
    println!("Usage: {} infile", prog);
    exit(1);
}

fn main() {
    let args: Vec<String> = args().collect();

    if args.len() != 2 {
        usage(args[0].clone())
    }

    let mut compiler = Compiler::new(&args[1]);
    compiler.run();
}