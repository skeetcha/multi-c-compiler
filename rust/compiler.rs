use std::io::Read;
use std::fs::File;

#[derive(Clone)]
enum Token {
    Eof,
    Plus,
    Minus,
    Star,
    Slash,
    IntLit(i64)
}

impl std::fmt::Display for Token {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Token::Eof => write!(f, "Token EOF"),
            Token::Plus => write!(f, "Token +"),
            Token::Minus => write!(f, "Token -"),
            Token::Star => write!(f, "Token *"),
            Token::Slash => write!(f, "Token /"),
            Token::IntLit(val) => write!(f, "Token intlit, value {}", val)
        }
    }
}

impl Token {
    fn op_prec(&self) -> u32 {
        match self {
            Self::Eof => 0,
            Self::Plus => 10,
            Self::Minus => 10,
            Self::Star => 20,
            Self::Slash => 20,
            Self::IntLit(_) => 0
        }
    }
}

enum ASTNodeType {
    Add,
    Subtract,
    Multiply,
    Divide,
    IntLit(i64)
}

impl std::fmt::Display for ASTNodeType {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            ASTNodeType::Add => write!(f, "+"),
            ASTNodeType::Subtract => write!(f, "-"),
            ASTNodeType::Multiply => write!(f, "*"),
            ASTNodeType::Divide => write!(f, "/"),
            ASTNodeType::IntLit(val) => write!(f, "int {}", val)
        }
    }
}

struct ASTNode {
    op: ASTNodeType,
    left: Option<Box<ASTNode>>,
    right: Option<Box<ASTNode>>
}

impl ASTNode {
    fn mkASTNode(op: ASTNodeType, left: Option<Box<ASTNode>>, right: Option<Box<ASTNode>>) -> Self {
        Self {
            op: op,
            left: left,
            right: right
        }
    }

    fn mkAstLeaf(op: ASTNodeType) -> Self {
        Self::mkASTNode(op, None, None)
    }

    fn mkAstUnary(op: ASTNodeType, left: Box<ASTNode>) -> Self {
        Self::mkASTNode(op, Some(left), None)
    }
}

struct Compiler {
    putback: char,
    line: i64,
    in_data: Vec<u8>,
    index: usize,
    token: Token
}

fn chrpos(s: &str, c: char) -> i64 {
    match s.chars().position(|ch| ch == c) {
        Some(val) => val as i64,
        None => -1
    }
}

impl Compiler {
    fn next(&mut self) -> Option<char> {
        let c: Option<char>;

        if self.putback != '\0' {
            c = Some(self.putback);
            self.putback = '\0';
            return c;
        }
        
        c = self.read();

        if let Some('\n') = c {
            self.line += 1;
        }

        return c;
    }

    fn read(&mut self) -> Option<char> {
        if self.index < self.in_data.len() {
            let c: char = self.in_data[self.index] as char;
            self.index += 1;
            Some(c)
        } else {
            None
        }
    }

    fn skip(&mut self) -> Option<char> {
        let c: Option<char> = self.next();

        match c {
            Some(val) => {
                let mut vval = val;

                while (vval == ' ') || (vval == '\t') || (vval == '\n') || (vval == '\r') {
                    let new_char: Option<char> = self.next();

                    match new_char {
                        Some(new_val) => vval = new_val,
                        None => return None
                    }
                }

                Some(vval)
            },
            None => c
        }
    }

    fn scan(&mut self) -> bool {
        let c: Option<char> = self.skip();

        match c {
            Some(cval) => match cval {
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
                    if cval.is_digit(10) {
                        self.token = Token::IntLit(self.scanint(cval));
                        true
                    } else {
                        panic!("Unrecognized character {} on line {}", c.unwrap_or('\0'), self.line);
                    }
                }
            },
            None => {
                self.token = Token::Eof;
                false
            }
        }
    }

    fn scanint(&mut self, c: char) -> i64 {
        let mut k: i64 = chrpos("0123456789", c);
        let mut val: i64 = 0;
        let mut ch: Option<char> = Some(c);

        while k >= 0 {
            val = val * 10 + k;
            ch = self.next();
            k = chrpos("0123456789", ch.unwrap_or('\0'));
        }

        self.putback = ch.unwrap_or('\0');
        return val;
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

    fn arithop(&self, token: Token) -> ASTNodeType {
        match token {
            Token::Plus => ASTNodeType::Add,
            Token::Minus => ASTNodeType::Subtract,
            Token::Star => ASTNodeType::Multiply,
            Token::Slash => ASTNodeType::Divide,
            _ => panic!("unknown token in arithop() on line {}", self.line)
        }
    }

    fn primary(&mut self) -> ASTNode {
        match self.token {
            Token::IntLit(val) => {
                let node = ASTNode::mkAstLeaf(ASTNodeType::IntLit(val));
                self.scan();
                node
            },
            _ => panic!("syntax error on line {}", self.line)
        }
    }

    fn check_bin_op(&self, tok: Token) -> u32 {
        let prec = tok.op_prec();

        if prec == 0 {
            panic!("syntax error on line {}, token {}", self.line, tok);
        }

        return prec;
    }

    fn binexpr(&mut self, ptp: u32) -> ASTNode {
        let mut left = self.primary();
        let mut token = self.token.clone();

        if let Token::Eof = token {
            return left;
        }

        while self.check_bin_op(token.clone()) > ptp {
            self.scan();
            let right = self.binexpr(token.op_prec());
            left = ASTNode::mkASTNode(self.arithop(token), Some(Box::new(left)), Some(Box::new(right)));
            token = self.token.clone();

            if let Token::Eof = token {
                return left;
            }
        }

        left
    }

    fn interpret_ast(node: ASTNode) -> i64 {
        let mut left_val: i64 = 0;
        let mut right_val: i64 = 0;

        if let Some(left_node) = node.left {
            left_val = Self::interpret_ast(*left_node);
        }

        if let Some(right_node) = node.right {
            right_val = Self::interpret_ast(*right_node);
        }

        if let ASTNodeType::IntLit(val) = node.op {
            println!("int {}", val);
        } else {
            println!("{} {} {}", left_val, node.op, right_val);
        }

        match node.op {
            ASTNodeType::Add => left_val + right_val,
            ASTNodeType::Subtract => left_val - right_val,
            ASTNodeType::Multiply => left_val * right_val,
            ASTNodeType::Divide => left_val / right_val,
            ASTNodeType::IntLit(val) => val,
        }
    }
}

fn usage(prog: String) {
    println!("Usage: {} infile", prog);
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() != 2 {
        usage(args[0].clone());
        return;
    }

    let mut compiler = Compiler::new(&args[1]);
    compiler.scan();
    let node = compiler.binexpr(0);
    println!("{}", Compiler::interpret_ast(node));
}