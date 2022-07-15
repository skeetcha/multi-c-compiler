use std::io::{Read, Write};
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

#[derive(Clone)]
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

#[derive(Clone)]
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
    token: Token,
    out_file: File,
    free_reg: [i32; 4],
    reg_list: [String; 4]
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
        let mut out_file = File::create("out.s").expect("unable to open out.s");

        Self {
            line: 1,
            putback: '\n',
            in_data: buffer.to_owned(),
            index: 0,
            token: Token::Eof,
            out_file: out_file,
            free_reg: [0i32; 4],
            reg_list: ["%r8".into(), "%r9".into(), "%r10".into(), "%r11".into()]
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

    fn gen_ast(&mut self, node: ASTNode) -> i64 {
        let mut left_reg: i64 = 0;
        let mut right_reg: i64 = 0;

        if let Some(left_node) = node.left {
            left_reg = self.gen_ast(*left_node);
        }

        if let Some(right_node) = node.right {
            right_reg = self.gen_ast(*right_node);
        }

        match node.op {
            ASTNodeType::Add => self.cg_add(left_reg, right_reg),
            ASTNodeType::Subtract => self.cg_sub(left_reg, right_reg),
            ASTNodeType::Multiply => self.cg_mul(left_reg, right_reg),
            ASTNodeType::Divide => self.cg_div(left_reg, right_reg),
            ASTNodeType::IntLit(val) => self.cg_load(val),
        }
    }

    fn cg_add(&mut self, left_reg: i64, right_reg: i64) -> i64 {
        self.out_file.write_all(&(&format!("\taddq\t{}, {}\n", self.reg_list[left_reg as usize], self.reg_list[right_reg as usize])).as_bytes()).expect("Couldn't write to out file");
        self.free_register(left_reg);
        right_reg
    }

    fn cg_sub(&mut self, left_reg: i64, right_reg: i64) -> i64 {
        self.out_file.write_all(&(&format!("\tsubq\t{}, {}\n", self.reg_list[right_reg as usize], self.reg_list[left_reg as usize])).as_bytes()).expect("Couldn't write to out file");
        self.free_register(right_reg);
        left_reg
    }
    
    fn cg_mul(&mut self, left_reg: i64, right_reg: i64) -> i64 {
        self.out_file.write_all(&(&format!("\timulq\t{}, {}\n", self.reg_list[left_reg as usize], self.reg_list[right_reg as usize])).as_bytes()).expect("Couldn't write to out file");
        self.free_register(left_reg);
        right_reg
    }

    fn cg_div(&mut self, left_reg: i64, right_reg: i64) -> i64 {
        self.out_file.write_all(&(&format!("\tmovq\t{},%rax\n\tcqo\n\tidivq\t{}\n\tmovq\t%rax,{}\n", self.reg_list[left_reg as usize], self.reg_list[right_reg as usize], self.reg_list[left_reg as usize])).as_bytes()).expect("Couldn't write to out file");
        self.free_register(right_reg);
        left_reg
    }

    fn cg_load(&mut self, val: i64) -> i64 {
        let r = self.alloc_register();
        self.out_file.write_all(&(&format!("\tmovq\t${}, {}\n", val, self.reg_list[r as usize])).as_bytes()).expect("Couldn't write to out file");
        r
    }

    fn generate_code(&mut self, node: ASTNode) {
        self.cg_preamble();
        let reg = self.gen_ast(node);
        self.cg_printint(reg);
        self.cg_postamble();
    }

    fn cg_preamble(&mut self) {
        self.free_all_registers();
        self.out_file.write_all(&("\t.text\n.LC0:\n\t.string\t\"%d\\n\"\nprintint:\n\tpushq\t%rbp\n\tmovq\t%rsp, %rbp\n\tsubq\t$16, %rsp\n\tmovl\t%edi, -4(%rbp)\n\tmovl\t-4(%rbp), %eax\n\tmovl\t%eax, %esi\n\tleaq\t.LC0(%rip), %rdi\n\tmovl\t$0, %eax\n\tcall\tprintf@PLT\n\tnop\n\tleave\n\tret\n\n\t.globl\tmain\n\t.type\tmain, @function\nmain:\n\tpushq\t%rbp\n\tmovq\t%rsp, %rbp\n").as_bytes()).expect("Couldn't write to out file");
    }

    fn cg_printint(&mut self, reg: i64) {
        self.out_file.write_all(&(&format!("\tmovq\t{}, %rdi\n\tcall\tprintint\n", self.reg_list[reg as usize])).as_bytes()).expect("Couldn't write to out file");
        self.free_register(reg);
    }

    fn cg_postamble(&mut self) {
        self.out_file.write_all(&("\tmovl\t$0, %eax\n\tpopq\t%rbp\n\tret\n").as_bytes()).expect("Couldn't write to out file");
    }

    fn free_all_registers(&mut self) {
        for i in 0..4 {
            self.free_reg[i] = 1;
        }
    }

    fn alloc_register(&mut self) -> i64 {
        for i in 0..4 {
            if self.free_reg[i] != 0 {
                self.free_reg[i] = 0;
                return i as i64;
            }
        }

        panic!("Out of registers!");
    }

    fn free_register(&mut self, reg: i64) {
        if self.free_reg[reg as usize] != 0 {
            panic!("Error trying to free register {}", reg);
        }

        self.free_reg[reg as usize] = 1;
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
    println!("{}", Compiler::interpret_ast(node.clone()));
    compiler.generate_code(node);
}