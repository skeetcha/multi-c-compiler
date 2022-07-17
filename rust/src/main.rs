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

    fn mk_ast_node(op: ASTNodeOp) -> Self {
        Self {
            op: op,
            left: None,
            right: None
        }
    }

    fn mk_ast_unary(op: ASTNodeOp, left: Box<ASTNode>) -> Self {
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
    token: Token,
    module_ctx: Context
}

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
            token: Token::Eof,
            module_ctx: Context::create()
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
            let node = ASTNode::mk_ast_node(ASTNodeOp::IntLit(val));
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

        loop {
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

        while (matches!(token, Token::Star)) || (matches!(token, Token::Slash)) {
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

    fn build_ast(&'a self, node: ASTNode, builder: &'a Builder) -> BasicValueEnum<'a> {
        let mut left_val: Option<BasicValueEnum> = None;
        let mut right_val: Option<BasicValueEnum> = None;

        if let Some(left_node) = node.left.clone() {
            left_val = Some(self.build_ast(*left_node, builder));
        }

        if let Some(right_node) = node.right.clone() {
            right_val = Some(self.build_ast(*right_node, builder));
        }

        match node.op.clone() {
            ASTNodeOp::Add => builder.build_int_add(left_val.unwrap().into_int_value(), right_val.unwrap().into_int_value(), "add").as_basic_value_enum(),
            ASTNodeOp::Subtract =>  builder.build_int_sub(left_val.unwrap().into_int_value(), right_val.unwrap().into_int_value(), "sub").as_basic_value_enum(),
            ASTNodeOp::Multiply => builder.build_int_mul(left_val.unwrap().into_int_value(), right_val.unwrap().into_int_value(), "mul").as_basic_value_enum(),
            ASTNodeOp::Divide => builder.build_int_signed_div(left_val.unwrap().into_int_value(), right_val.unwrap().into_int_value(), "div").as_basic_value_enum(),
            ASTNodeOp::IntLit(val) => {
                let val_ptr = builder.build_alloca(self.module_ctx.i32_type(), "int");
                builder.build_store(val_ptr, self.module_ctx.i32_type().const_int(val as u64, false));
                builder.build_load(val_ptr, "int_val")
            }
        }
    }

    fn gen_print(&'a self, builder: &'a Builder, val: BasicValueEnum<'a>, print_fn: FunctionValue<'a>) {
        let const_string = self.module_ctx.const_string(b"%d\n\0", true);
        let fmt_ptr = builder.build_alloca(const_string.get_type(), "fmt");
        builder.build_store(fmt_ptr, const_string);
        let fmt_string_ptr = builder.build_bitcast(fmt_ptr, self.module_ctx.i8_type().ptr_type(AddressSpace::Generic), "fmt_ptr");
        builder.build_call(print_fn, &[fmt_string_ptr.into(), val.into()], "print");
    }

    fn generate_code(&self, node: ASTNode, out_file: &Path) {
        let module = self.module_ctx.create_module("main_module");
        let fn_type = self.module_ctx.i32_type().fn_type(&[self.module_ctx.i32_type().into()], false);
        let main_fn = module.add_function("main", fn_type, None);
        let const_string = self.module_ctx.const_string(b"\0", true);
        let printf_fn_type = self.module_ctx.i32_type().fn_type(&[self.module_ctx.i8_type().ptr_type(AddressSpace::Generic).into()], true);
        let printf_fn = module.add_function("printf", printf_fn_type, Some(Linkage::External));
        let builder = self.module_ctx.create_builder();
        let entry = self.module_ctx.append_basic_block(main_fn, "entry");
        builder.position_at_end(entry);
        let ret_value = self.build_ast(node, &builder);
        self.gen_print(&builder, ret_value, printf_fn);
        let good_return = self.module_ctx.i32_type().const_int(0, false);
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

    fn run(&mut self) -> ASTNode {
        self.scan();
        self.expr()
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
    let node: ASTNode = compiler.run();
    let mut out_file = Path::new(".").join(Path::new(&args[1]).file_name().unwrap());
    out_file.set_extension("o");
    compiler.generate_code(node, out_file.as_path());
}