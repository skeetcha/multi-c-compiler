extern crate inkwell;

use std::{env::args, process::exit, fs::{File}, io::Read, sync::Arc, path::Path};
use inkwell::{module::{Module, Linkage}, context::Context, values::{BasicValueEnum, BasicValue, FunctionValue}, builder::Builder, targets::{InitializationConfig, Target, FileType, TargetMachine, RelocMode, CodeModel}, OptimizationLevel, AddressSpace};

#[derive(Clone)]
enum Token {
    Eof,
    Plus,
    Minus,
    Star,
    Slash,
    IntLit(i64),
    Semi,
    Print
}

impl std::fmt::Display for Token {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Token::Eof => write!(f, "EOF"),
            Token::Plus => write!(f, "+"),
            Token::Minus => write!(f, "-"),
            Token::Star => write!(f, "*"),
            Token::Slash => write!(f, "/"),
            Token::IntLit(val) => write!(f, "int {}", val),
            Token::Semi => write!(f, ";"),
            Token::Print => write!(f, "print")
        }
    }
}

enum ASTNode {
    Add(Box<ASTNode>, Box<ASTNode>),
    Sub(Box<ASTNode>, Box<ASTNode>),
    Mul(Box<ASTNode>, Box<ASTNode>),
    Div(Box<ASTNode>, Box<ASTNode>),
    IntLit(i64)
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

const TEXT_LEN_LIMIT: i64 = 512;

impl<'a> Compiler {
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
            ';' => {
                self.token = Token::Semi;
                true
            },
            _ => {
                if c.is_digit(10) {
                    self.token = Token::IntLit(self.scanint(c));
                    true
                } else if (c.is_alphabetic()) || (c == '_') {
                    let text = self.scanident(c);
                    let new_token = self.keyword(text.clone());
                    self.token = new_token.expect(&format!("Unrecognized symbol {} on line {}", text, self.line));
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

    fn scanident(&mut self, c: char) -> String {
        let mut i = 0;
        let mut buf: String = String::new();
        let mut ch: char = c;

        while (ch.is_alphabetic()) || (ch.is_digit(10)) || (ch == '_') {
            if i == (TEXT_LEN_LIMIT - 1) {
                panic!("identifier too long on line {}", self.line);
            } else if i < (TEXT_LEN_LIMIT - 1) {
                buf += &String::from(ch);
                i += 1;
            }

            ch = self.next();
        }

        self.putback = ch;
        buf += "\0";
        buf
    }

    fn keyword(&self, s: String) -> Option<Token> {
        if s == "print\0" {
            Some(Token::Print)
        } else {
            None
        }
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

    fn number(&mut self) -> ASTNode {
        if let Token::IntLit(val) = self.token {
            let node = ASTNode::IntLit(val);
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

        if matches!(token, Token::Semi) {
            return left;
        }

        loop {
            self.scan();
            let right = self.mul_expr();

            left = match token {
                Token::Plus => ASTNode::Add(Box::new(left), Box::new(right)),
                Token::Minus => ASTNode::Sub(Box::new(left), Box::new(right)),
                Token::Star => ASTNode::Mul(Box::new(left), Box::new(right)),
                Token::Slash => ASTNode::Div(Box::new(left), Box::new(right)),
                _ => {
                    panic!("Invalid token on line {}", self.line);
                }
            };
            
            token = self.token.clone();

            if matches!(token, Token::Semi) {
                break;
            }
        }

        left
    }

    fn mul_expr(&mut self) -> ASTNode {
        let mut left: ASTNode = self.number();
        let mut token: Token = self.token.clone();

        if matches!(token, Token::Semi) {
            return left;
        }

        while (matches!(token, Token::Star)) || (matches!(token, Token::Slash)) {
            match token.clone() {
                Token::Star => (),
                Token::Slash => (),
                _ => break
            }

            self.scan();
            let right = self.number();

            left = match token {
                Token::Plus => ASTNode::Add(Box::new(left), Box::new(right)),
                Token::Minus => ASTNode::Sub(Box::new(left), Box::new(right)),
                Token::Star => ASTNode::Mul(Box::new(left), Box::new(right)),
                Token::Slash => ASTNode::Div(Box::new(left), Box::new(right)),
                _ => {
                    panic!("Invalid token on line {}", self.line);
                }
            };

            token = self.token.clone();

            if matches!(token, Token::Semi) {
                break;
            }
        }

        left
    }

    fn match_token(&mut self, ttoken: Token, tstr: &str) {
        if matches!(&self.token, ttoken) {
            self.scan();
        } else {
            panic!("{} expected on line {}", tstr, self.line);
        }
    }

    fn semi(&mut self) {
        self.match_token(Token::Semi, ";");
    }

    fn statements(&'a mut self, builder: &'a Builder, printf_fn: FunctionValue<'a>, module_ctx: &'a Context) {
        loop {
            self.match_token(Token::Print, "print");
            let tree = self.expr();
            let ret_val = self.build_ast(tree, builder, module_ctx);
            self.gen_print(builder, ret_val, printf_fn, module_ctx);
            self.semi();

            if matches!(self.token, Token::Eof) {
                return;
            }
        }
    }

    fn build_ast(&'a self, node: ASTNode, builder: &'a Builder, module_ctx: &'a Context) -> BasicValueEnum<'a> {
        match node {
            ASTNode::Add(left, right) => builder.build_int_add(self.build_ast(*left, builder, module_ctx).into_int_value(), self.build_ast(*right, builder, module_ctx).into_int_value(), "add").as_basic_value_enum(),
            ASTNode::Sub(left, right) =>  builder.build_int_sub(self.build_ast(*left, builder, module_ctx).into_int_value(), self.build_ast(*right, builder, module_ctx).into_int_value(), "sub").as_basic_value_enum(),
            ASTNode::Mul(left, right) => builder.build_int_mul(self.build_ast(*left, builder, module_ctx).into_int_value(), self.build_ast(*right, builder, module_ctx).into_int_value(), "mul").as_basic_value_enum(),
            ASTNode::Div(left, right) => builder.build_int_signed_div(self.build_ast(*left, builder, module_ctx).into_int_value(), self.build_ast(*right, builder, module_ctx).into_int_value(), "div").as_basic_value_enum(),
            ASTNode::IntLit(val) => {
                let val_ptr = builder.build_alloca(module_ctx.i32_type(), "int");
                builder.build_store(val_ptr, module_ctx.i32_type().const_int(val as u64, false));
                builder.build_load(val_ptr, "int_val")
            }
        }
    }

    fn gen_print(&'a self, builder: &'a Builder, val: BasicValueEnum<'a>, print_fn: FunctionValue<'a>, module_ctx: &'a Context) {
        let const_string = module_ctx.const_string(b"%d\n\0", true);
        let fmt_ptr = builder.build_alloca(const_string.get_type(), "fmt");
        builder.build_store(fmt_ptr, const_string);
        let fmt_string_ptr = builder.build_bitcast(fmt_ptr, module_ctx.i8_type().ptr_type(AddressSpace::Generic), "fmt_ptr");
        builder.build_call(print_fn, &[fmt_string_ptr.into(), val.into()], "print");
    }

    fn generate_code(&mut self, out_file: &Path) {
        let module_ctx = Context::create();
        let module = module_ctx.create_module("main_module");
        let fn_type = module_ctx.i32_type().fn_type(&[module_ctx.i32_type().into()], false);
        let main_fn = module.add_function("main", fn_type, None);
        let printf_fn_type = module_ctx.i32_type().fn_type(&[module_ctx.i8_type().ptr_type(AddressSpace::Generic).into()], true);
        let printf_fn = module.add_function("printf", printf_fn_type, Some(Linkage::External));
        let builder = module_ctx.create_builder();
        let entry = module_ctx.append_basic_block(main_fn, "entry");
        builder.position_at_end(entry);
        self.statements(&builder, printf_fn, &module_ctx);
        let good_return = module_ctx.i32_type().const_int(0, false);
        builder.build_return(Some(&good_return));
        module.set_triple(&TargetMachine::get_default_triple());
        let res = module.verify();

        if let Err(errors) = res {
            panic!("{}", errors);
        }
        
        Target::initialize_native(&InitializationConfig::default()).expect("Failed to initialize native target");
        let cpu_name = TargetMachine::get_host_cpu_name();
        let cpu_features = TargetMachine::get_host_cpu_features();
        let target = Target::from_triple(&module.get_triple()).unwrap();
        let machine = target.create_target_machine(&module.get_triple(), &cpu_name.to_string(), &cpu_features.to_string(), OptimizationLevel::None, RelocMode::Default, CodeModel::Default).unwrap();
        let ass_res = machine.write_to_file(&module, FileType::Object, out_file);

        if let Err(errors) = ass_res {
            panic!("{}", errors);
        }
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

    let mut compiler = Compiler::new(&args[1].clone());
    compiler.scan();
    let mut out_file = Path::new(".").join(Path::new(&args[1]).file_name().unwrap());
    out_file.set_extension("o");
    compiler.generate_code(out_file.as_path());
}